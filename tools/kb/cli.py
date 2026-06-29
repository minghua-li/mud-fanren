from __future__ import annotations
import argparse
import sys
from pathlib import Path

from tools.kb.core.repo import kb_root


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(prog="python -m tools.kb", description="KB 知识库管理工具")
    sub = parser.add_subparsers(dest="cmd", required=True)

    # new
    p = sub.add_parser("new", help="Scaffold 新 entry")
    p.add_argument("id", help="kebab-case id")
    p.add_argument("--cluster", required=True, help="cluster 目录名")

    # search
    p = sub.add_parser("search", help="多轴检索")
    p.add_argument("terms", nargs="*", help="搜索关键词")
    p.add_argument("--tag", help="按 tag 过滤")
    p.add_argument("--module", help="按 module 过滤")
    p.add_argument("--kind", help="按 kind 过滤")
    p.add_argument("--limit", type=int, default=20)

    # show
    p = sub.add_parser("show", help="显示 entry frontmatter + claim")
    p.add_argument("id", help="entry id")

    # validate
    sub.add_parser("validate", help="校验所有 entry")

    # gen-indexes
    sub.add_parser("gen-indexes", help="重新生成 INDEX-*.md")

    # tags
    p = sub.add_parser("tags", help="列 tag 统计")
    p.add_argument("--unused", action="store_true")
    p.add_argument("--orphan", action="store_true")

    # modules
    p = sub.add_parser("modules", help="列 module 统计")
    p.add_argument("--unused", action="store_true")
    p.add_argument("--orphan", action="store_true")

    # tag (atomic add/remove)
    p = sub.add_parser("tag", help="原子增删 entry tag")
    p.add_argument("id", help="entry id")
    p.add_argument("changes", nargs="+", help='+tag 或 -tag')

    # relate
    p = sub.add_parser("relate", help="双向 related 链接")
    p.add_argument("id1", help="entry id 1")
    p.add_argument("id2", help="entry id 2")
    p.add_argument("--unlink", action="store_true", help="取消链接")

    # supersede
    p = sub.add_parser("supersede", help="建立 supersession")
    p.add_argument("old", help="旧 entry id")
    p.add_argument("new", help="新 entry id")

    # stale
    p = sub.add_parser("stale", help="列出超期 entry")
    p.add_argument("--months", type=int, default=6)

    # verify
    p = sub.add_parser("verify", help="更新 verified 日期")
    p.add_argument("id", help="entry id")
    p.add_argument("--date", help="YYYY-MM-DD, 默认今天")

    # append
    p = sub.add_parser("append", help="追加正文章节")
    p.add_argument("id", help="entry id")
    p.add_argument("--section", required=True, help="章节标题")

    # graph
    p = sub.add_parser("graph", help="关系图")
    p.add_argument("id", help="center entry id")
    p.add_argument("--depth", type=int, default=2)
    p.add_argument("--format", choices=["dot", "ascii"], default="ascii")

    # similar
    p = sub.add_parser("similar", help="找相似 entry")
    p.add_argument("id", help="entry id")
    p.add_argument("--limit", type=int, default=5)

    return parser


def main(argv: list[str] | None = None) -> int:
    if argv is None:
        argv = sys.argv[1:]

    parser = build_parser()
    args = parser.parse_args(argv)

    try:
        root = kb_root()
    except Exception as e:
        print(f"错误: 无法找到 KB 根目录 - {e}", file=sys.stderr)
        return 1

    cmd = args.cmd

    if cmd == "new":
        from tools.kb.commands.new import cmd_new_main
        return cmd_new_main(root, args)
    elif cmd == "search":
        from tools.kb.commands.search import cmd_search_main
        return cmd_search_main(root, args)
    elif cmd == "show":
        from tools.kb.commands.show import cmd_show_main
        return cmd_show_main(root, args)
    elif cmd == "validate":
        from tools.kb.commands.validate import cmd_validate_main
        return cmd_validate_main(root, args)
    elif cmd == "gen-indexes":
        from tools.kb.commands.gen_indexes import cmd_gen_indexes_main
        return cmd_gen_indexes_main(root, args)
    elif cmd == "tags":
        from tools.kb.commands.tags import cmd_tags_main
        return cmd_tags_main(root, args)
    elif cmd == "modules":
        from tools.kb.commands.modules import cmd_modules_main
        return cmd_modules_main(root, args)
    elif cmd == "tag":
        from tools.kb.commands.tag import cmd_tag_main
        return cmd_tag_main(root, args)
    elif cmd == "relate":
        from tools.kb.commands.relate import cmd_relate_main
        return cmd_relate_main(root, args)
    elif cmd == "supersede":
        from tools.kb.commands.supersede import cmd_supersede_main
        return cmd_supersede_main(root, args)
    elif cmd == "stale":
        from tools.kb.commands.stale import cmd_stale_main
        return cmd_stale_main(root, args)
    elif cmd == "verify":
        from tools.kb.commands.verify import cmd_verify_main
        return cmd_verify_main(root, args)
    elif cmd == "append":
        from tools.kb.commands.append import cmd_append_main
        return cmd_append_main(root, args)
    elif cmd == "graph":
        from tools.kb.commands.graph import cmd_graph_main
        return cmd_graph_main(root, args)
    elif cmd == "similar":
        from tools.kb.commands.similar import cmd_similar_main
        return cmd_similar_main(root, args)

    return 0
