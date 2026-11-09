# Copyright (c) 2026, vkTerra Contributors
# SPDX-License-Identifier: MPL-2.0
# See LICENSE-MPL-2.0.txt for more information.

import patch_ng
import sys

if __name__ == "__main__":
    patches = []
    for path in sys.argv[1:]:
        patch = patch_ng.fromfile(path)
        if not patch:
            print(f"Failed to parse patch: {path}", file=sys.stderr)
            sys.exit(1)
        patches.append(patch)
    for patch in patches:
        if not patch.apply():
            print(f"Failed to apply patch: {patch}", file=sys.stderr)
            sys.exit(1)
