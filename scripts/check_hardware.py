#!/usr/bin/env python3
"""Cross-check source and compiled devicetree against independently captured PCB1."""
import argparse
import json
import pickle
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
EVIDENCE = json.loads((ROOT/"hardware/pcb1.json").read_text(encoding="utf-8"))
SHIELD = ROOT/"boards/shields/gridrolldg"

def net_gpio(side, net):
    module = "U3" if side == "left" else "U4"
    return next(p["gpio"] for p in EVIDENCE["pads"][module] if p["net"] == net)

def expected_matrix(side):
    rows = [net_gpio(side, f"ROW{i}"+("R" if side == "right" else "")) for i in range(4)]
    cols = [net_gpio(side, f"COL{i}") for i in range(0 if side == "left" else 6, 6 if side == "left" else 12)]
    return rows, cols

def source_checks():
    expected = [(k["row"], k["col"]) for k in EVIDENCE["keys"]]
    assert len(expected) == len(set(expected)) == 47
    assert (3, 8) not in expected and (3, 3) in expected
    layout = (SHIELD/"gridrolldg-layouts.dtsi").read_text(encoding="utf-8")
    actual = [tuple(map(int, m)) for m in re.findall(r"RC\((\d+),(\d+)\)", layout)]
    assert actual == expected, "Matrix transform does not match PCB1"
    assert layout.count("&key_physical_attrs") == 47, "Wrong Studio key count"
    for side in ("left", "right"):
        overlay = (SHIELD/f"gridrolldg_{side}.overlay").read_text(encoding="utf-8")
        for prop, expected_pins in zip(("row-gpios", "col-gpios"), expected_matrix(side)):
            value = re.search(prop+r"\s*=([^;]+);", overlay).group(1)
            actual = [list(map(int, p)) for p in re.findall(r"&gpio([01])\s+(\d+)", value)]
            assert actual == expected_pins, f"{side} {prop}: {actual} != {expected_pins}"
    keymap = (ROOT/"config/gridrolldg.keymap").read_text(encoding="utf-8")
    bindings = re.findall(r"^\s*bindings\s*=\s*<([^>]+)>", keymap.split("keymap {", 1)[1], re.M)
    assert len(bindings) == 4 and all(len(re.findall(r"&\w+", b)) == 47 for b in bindings)
    common = (SHIELD/"gridrolldg.dtsi").read_text(encoding="utf-8")
    assert 'diode-direction = "col2row"' in common
    assert "steps = <60>" in common and "triggers-per-rotation = <30>" in common
    board = (ROOT/"boards/arm/gridrolldg_e73/gridrolldg_e73.dts").read_text(encoding="utf-8")
    for value in ("io-channels = <&adc 2>", "output-ohms = <2000000>", "full-ohms = <2820000>"):
        assert value in board
    print("PCB1 source GPIOs, 47-key layout, diode direction, ADC and encoder checked")

def compiled_checks(build, target):
    workspace = build.parent.parent
    sys.path.insert(0, str(workspace/"zephyr/scripts/dts/python-devicetree/src"))
    with (build/"zephyr/edt.pickle").open("rb") as stream:
        edt = pickle.load(stream)
    config = (build/"zephyr/.config").read_text(encoding="utf-8")
    for flag in ("CONFIG_NFCT_PINS_AS_GPIOS=y", "CONFIG_GPIO_AS_PINRESET=y",
                 "CONFIG_CLOCK_CONTROL_NRF_K32SRC_RC=y"):
        assert flag in config, f"{target}: missing {flag}"
    if target == "settings-reset":
        return
    side = "left" if target == "left" else "right"
    def nodes(compatible):
        return [n for n in edt.nodes if compatible in n.compats and n.status == "okay"]
    def gpio(spec):
        label = next(l for l in spec.controller.labels if l in ("gpio0", "gpio1"))
        return [int(label[-1]), spec.data["pin"]]
    kscan, = nodes("zmk,kscan-gpio-matrix")
    used = []
    for prop, expected in zip(("row-gpios", "col-gpios"), expected_matrix(side)):
        actual = [gpio(s) for s in kscan.props[prop].val]
        assert actual == expected, f"Compiled {side} {prop} mismatch"
        used += actual
    assert kscan.props["diode-direction"].val == "col2row"
    physical, = nodes("zmk,physical-layout")
    assert len(physical.props["keys"].val) == 47
    transform = physical.props["transform"].val
    expected = [(k["row"] << 8) | k["col"] for k in EVIDENCE["keys"]]
    assert transform.props["map"].val == expected
    assert transform.props.get("col-offset").val == (0 if side == "left" else 6)
    battery, = nodes("zmk,battery-voltage-divider")
    assert battery.props["io-channels"].val[0].data["input"] == 2
    assert battery.props["output-ohms"].val == 2000000
    assert battery.props["full-ohms"].val == 2820000
    used.append([0, 4])
    if side == "left":
        assert "CONFIG_ZMK_SPLIT_ROLE_CENTRAL=y" not in config
        assert "CONFIG_ZMK_STUDIO=y" not in config
        assert not nodes("pandakb,paw3222-sdio")
        encoder, = nodes("alps,ec11")
        assert encoder.props["steps"].val == 60
        for prop, net in (("a-gpios", "ECA"), ("b-gpios", "ECB")):
            pin = gpio(encoder.props[prop].val[0])
            assert pin == net_gpio(side, net)
            used.append(pin)
        sensors, = nodes("zmk,keymap-sensors")
        assert sensors.props["triggers-per-rotation"].val == 30
    else:
        for flag in ("CONFIG_ZMK_SPLIT_ROLE_CENTRAL=y", "CONFIG_ZMK_STUDIO=y",
                     "CONFIG_USB_CDC_ACM=y", "CONFIG_GRIDROLLDG_PAW3222=y",
                     "CONFIG_ZMK_POINTING=y"):
            assert flag in config, f"{target}: missing {flag}"
        assert not nodes("alps,ec11")
        sensor, = nodes("pandakb,paw3222-sdio")
        for prop, net in (("sdio-gpios", "SDIO"), ("sclk-gpios", "SCLK"),
                          ("cs-gpios", "NCS"), ("motion-gpios", "MOTION")):
            pin = gpio(sensor.props[prop].val[0])
            assert pin == net_gpio(side, net)
            used.append(pin)
        assert sensor.props["res-cpi"].val == 988
        assert sensor.props["poll-interval-ms"].val == 5
    assert len(used) == len({tuple(p) for p in used}), "Active GPIO functions overlap"
    print(f"{target}: compiled GPIOs, role, sensor, Studio and battery validated")

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--build", type=Path)
    parser.add_argument("--target", choices=["left", "right", "right-diagnostics", "settings-reset"])
    args = parser.parse_args()
    source_checks()
    if args.build:
        assert args.target
        compiled_checks(args.build.resolve(), args.target)

if __name__ == "__main__":
    main()
