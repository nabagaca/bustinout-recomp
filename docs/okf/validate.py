"""Check this flat OKF bundle and local references. Requires PyYAML."""
from pathlib import Path
from urllib.parse import unquote, urlparse
import re
import sys

try:
    import yaml
except ImportError:
    raise SystemExit("Documentation validation requires PyYAML; no application dependency is needed.")


def main():
    root = Path(__file__).resolve().parent
    documents = sorted(root.rglob("*.md"))
    errors, edges = [], {}
    references = 0

    def target(document, value):
        nonlocal references
        if urlparse(value).scheme or value.startswith("#"):
            return None
        path = unquote(value.split("#", 1)[0])
        resolved = (root / path.lstrip("/") if path.startswith("/") else document.parent / path).resolve()
        references += 1
        if not resolved.exists():
            errors.append(f"{document.name}: missing target {value}")
        return resolved

    for document in documents:
        text = document.read_text(encoding="utf-8")
        match = re.match(r"\A---\r?\n(.*?)\r?\n---(?:\r?\n|$)", text, re.S)
        metadata = {}
        if match:
            try:
                metadata = yaml.safe_load(match.group(1))
                if not isinstance(metadata, dict):
                    raise ValueError("frontmatter must be a mapping")
            except (yaml.YAMLError, ValueError) as exc:
                errors.append(f"{document.name}: invalid YAML: {exc}")
                metadata = {}
        if document.name == "index.md":
            allowed = {"okf_version"} if document == root / "index.md" else set()
            if set(metadata) - allowed or (match and not allowed):
                errors.append(f"{document.name}: reserved index frontmatter")
            if document == root / "index.md" and metadata.get("okf_version") != "0.2":
                errors.append("root index must declare okf_version 0.2")
        elif document.name == "log.md":
            if match:
                errors.append("log.md must not have frontmatter")
        elif not match or not isinstance(metadata.get("type"), str) or not metadata["type"].strip():
            errors.append(f"{document.name}: missing nonempty type/frontmatter")

        for source in metadata.get("sources", []):
            if not isinstance(source, dict) or not source.get("resource"):
                errors.append(f"{document.name}: source missing resource")
            else:
                target(document, source["resource"])
        body = text[match.end():] if match else text
        body = re.sub(r"```.*?```", "", body, flags=re.S)
        links = re.findall(r"(?<!!)\[[^\]]+\]\(([^\s)]+)\)", body)
        edges[document.resolve()] = {item for link in links if (item := target(document, link))}

    index = (root / "index.md").resolve()
    visited, pending = set(), [index]
    while pending:
        current = pending.pop()
        if current not in visited:
            visited.add(current)
            pending.extend(edges.get(current, set()) - visited)
    for document in documents:
        if document.name not in {"index.md", "log.md"}:
            if document.resolve() not in visited:
                errors.append(f"{document.name}: unreachable from root index")
            if document.resolve() not in edges.get(index, set()):
                errors.append(f"{document.name}: absent from root index")
    if errors:
        print("\n".join(errors), file=sys.stderr)
        return 1
    print(f"PASS: {len(documents)} Markdown documents, {references} local references; YAML, metadata, index coverage and reachability valid.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
