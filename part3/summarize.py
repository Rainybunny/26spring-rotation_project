"""
Usage: python summarize.py <test_name>
"""

import os
import re
import sys
from collections import defaultdict
from pathlib import Path


RESULT_LOG_CANDIDATES = ["_results.log", "_result.log", "results.log", "result.log"]


def parse_targets(test_dir: Path) -> list[str]:
	targets_file = test_dir / "targets.txt"
	if not targets_file.exists():
		raise FileNotFoundError(f"targets.txt not found in {test_dir}")

	targets: list[str] = []
	with open(targets_file, "r", encoding="utf-8") as f:
		for line in f:
			line = line.strip()
			if not line:
				continue
			parts = line.split()
			if len(parts) < 2:
				continue
			repo_name, commit_hash = parts[0], parts[1]
			targets.append(f"{repo_name}/{commit_hash}")
	return targets


def find_result_log(opt_dir: Path) -> Path | None:
	for file_name in RESULT_LOG_CANDIDATES:
		candidate = opt_dir / file_name
		if candidate.exists() and candidate.is_file():
			return candidate
	return None


def discover_optimizers(test_dir: Path) -> list[tuple[str, Path]]:
	optimizers: list[tuple[str, Path]] = []
	for entry in sorted(test_dir.iterdir()):
		if not entry.is_dir():
			continue
		result_log = find_result_log(entry)
		if result_log is not None:
			optimizers.append((entry.name, result_log))
	return optimizers


def parse_result_log(log_path: Path) -> tuple[dict[str, str], str | None]:
	details: dict[str, str] = {}
	status_line: str | None = None

	detail_pattern = re.compile(r"^>\s+([^\s:]+):\s*([AR])\b")
	status_pattern = re.compile(r"^status:\s*([AR]+)")

	with open(log_path, "r", encoding="utf-8", errors="replace") as f:
		for raw_line in f:
			line = raw_line.strip()
			if not line:
				continue

			m_status = status_pattern.match(line)
			if m_status:
				status_line = m_status.group(1)
				continue

			m_detail = detail_pattern.match(line)
			if m_detail:
				commit_key = m_detail.group(1)
				details[commit_key] = m_detail.group(2)

	return details, status_line


def status_for_targets(targets: list[str], details: dict[str, str]) -> list[str]:
	return [details.get(target, "?") for target in targets]


def format_accuracy_table(optimizer_status: dict[str, list[str]], total_targets: int) -> str:
	rows = []
	for opt_id, status_list in optimizer_status.items():
		accepted = sum(1 for s in status_list if s == "A")
		known = sum(1 for s in status_list if s in ("A", "R"))
		unknown = total_targets - known
		accuracy = (accepted / total_targets * 100.0) if total_targets else 0.0
		rows.append((opt_id, accepted, total_targets, accuracy, unknown))

	# Compute column widths for clean plain-text table output.
	headers = ("optimizer", "accepted", "total", "accuracy", "unknown")
	col_widths = [
		max(len(headers[0]), max((len(r[0]) for r in rows), default=0)),
		max(len(headers[1]), max((len(str(r[1])) for r in rows), default=0)),
		max(len(headers[2]), max((len(str(r[2])) for r in rows), default=0)),
		max(len(headers[3]), max((len(f"{r[3]:.2f}%") for r in rows), default=0)),
		max(len(headers[4]), max((len(str(r[4])) for r in rows), default=0)),
	]

	def fmt_row(values: tuple[str, str, str, str, str]) -> str:
		return (
			f"{values[0]:<{col_widths[0]}}  "
			f"{values[1]:>{col_widths[1]}}  "
			f"{values[2]:>{col_widths[2]}}  "
			f"{values[3]:>{col_widths[3]}}  "
			f"{values[4]:>{col_widths[4]}}"
		)

	lines = []
	lines.append(fmt_row(headers))
	lines.append(
		"-" * col_widths[0]
		+ "  "
		+ "-" * col_widths[1]
		+ "  "
		+ "-" * col_widths[2]
		+ "  "
		+ "-" * col_widths[3]
		+ "  "
		+ "-" * col_widths[4]
	)
	for opt_id, accepted, total, accuracy, unknown in rows:
		lines.append(
			fmt_row(
				(
					opt_id,
					str(accepted),
					str(total),
					f"{accuracy:.2f}%",
					str(unknown),
				)
			)
		)
	return "\n".join(lines)


def classify_status_patterns(
	targets: list[str], optimizer_order: list[str], optimizer_status: dict[str, list[str]]
) -> dict[str, list[str]]:
	classes: dict[str, list[str]] = defaultdict(list)
	target_count = len(targets)

	for idx in range(target_count):
		pattern = "".join(optimizer_status[opt_id][idx] for opt_id in optimizer_order)
		classes[pattern].append(targets[idx])

	return dict(classes)


def build_summary_text(
	test_name: str,
	targets: list[str],
	optimizer_order: list[str],
	optimizer_status: dict[str, list[str]],
	pattern_classes: dict[str, list[str]],
) -> str:
	lines: list[str] = []
	lines.append(f"Summary for test: {test_name}")
	lines.append(f"Total targets: {len(targets)}")
	lines.append(f"Optimizer order: {', '.join(optimizer_order)}")
	lines.append("")

	lines.append("=== Accuracy Table ===")
	lines.append(format_accuracy_table(optimizer_status, len(targets)))
	lines.append("")

	lines.append("=== Status Classes ===")
	lines.append("Pattern meaning follows optimizer order above.")
	lines.append("")

	sorted_classes = sorted(
		pattern_classes.items(),
		key=lambda kv: (-len(kv[1]), kv[0]),
	)
	for pattern, commits in sorted_classes:
		lines.append(f"[{pattern}] count={len(commits)}")
		for commit_key in commits:
			lines.append(f"- {commit_key}")
		lines.append("")

	return "\n".join(lines).rstrip() + "\n"


def main() -> int:
	if len(sys.argv) != 2:
		print("Usage: python summarize.py <test_name>", file=sys.stderr)
		return 1

	test_name = sys.argv[1]
	this_dir = Path(__file__).resolve().parent
	test_dir = this_dir / "tests" / test_name

	if not test_dir.exists() or not test_dir.is_dir():
		print(f"Test folder not found: {test_dir}", file=sys.stderr)
		return 1

	targets = parse_targets(test_dir)
	if not targets:
		print(f"No targets found in {test_dir / 'targets.txt'}", file=sys.stderr)
		return 1

	optimizers = discover_optimizers(test_dir)
	if not optimizers:
		print(
			f"No optimizer result logs found under {test_dir}. "
			f"Expected one of: {', '.join(RESULT_LOG_CANDIDATES)}",
			file=sys.stderr,
		)
		return 1

	optimizer_status: dict[str, list[str]] = {}
	optimizer_order: list[str] = []

	for opt_id, log_path in optimizers:
		details, _status_line = parse_result_log(log_path)
		optimizer_status[opt_id] = status_for_targets(targets, details)
		optimizer_order.append(opt_id)

	pattern_classes = classify_status_patterns(targets, optimizer_order, optimizer_status)
	summary_text = build_summary_text(
		test_name, targets, optimizer_order, optimizer_status, pattern_classes
	)

	summary_path = test_dir / "summary.log"
	with open(summary_path, "w", encoding="utf-8") as f:
		f.write(summary_text)

	print(f"Summary written to {summary_path}")
	return 0


if __name__ == "__main__":
	sys.exit(main())