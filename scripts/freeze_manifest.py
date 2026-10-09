#!/usr/bin/env python3
"""Freeze active modules; Zephyr lists inactive/uncloned optional projects too."""
import yaml
from west.manifest import Manifest

manifest = Manifest.from_topdir()
document = manifest.as_dict()
document["manifest"]["projects"] = []
for project in manifest.projects[1:]:
    if not manifest.is_active(project):
        continue
    if not project.is_cloned():
        raise RuntimeError(f"Active dependency {project.name} was not cloned")
    entry = project.as_dict()
    entry["revision"] = project.sha("manifest-rev")
    document["manifest"]["projects"].append(entry)
print(yaml.safe_dump(document, sort_keys=False), end="")
