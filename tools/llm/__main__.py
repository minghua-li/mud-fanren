from __future__ import annotations

"""python -m tools.llm 入口：等价于 python -m tools.llm.gateway。"""

import sys

from tools.llm.gateway import main

if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
