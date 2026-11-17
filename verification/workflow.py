#!/usr/bin/env python3
"""Verification workflow for one AI claim.

Collects the evidence, checks it is complete, and STOPS for a human decision.
It never commits, pushes, or opens/updates a PR by itself.

Usage (from the repository root):
    python3 verification/workflow.py verification/evidence.json
"""

from __future__ import annotations

import json
import sys
from pathlib import Path

REQUIRED = ("claim", "source_url", "test_command", "observed", "verdict")
VERDICTS = ("accept", "revise", "reject")


def main() -> int:
    path = Path(sys.argv[1] if len(sys.argv) > 1 else "verification/evidence.json")
    record = json.loads(path.read_text(encoding="utf-8"))

    # Step 1: is the evidence complete?
    missing = [field for field in REQUIRED[:-1] if not str(record.get(field, "")).strip()]
    if missing:
        print(f"INCOMPLETE: fill in {', '.join(missing)} before asking for a decision.")
        return 2

    print("Evidence collected")
    print(f"  claim:        {record['claim']}")
    print(f"  source:       {record['source_url']}")
    print(f"  test command: {record['test_command']}")
    print(f"  observed:     {record['observed']}")
    if record.get("uncertainty"):
        print(f"  uncertainty:  {record['uncertainty']}")

    # Step 2: HUMAN DECISION GATE. The script cannot continue without a person.
    verdict = str(record.get("verdict", "")).strip().lower()
    if verdict not in VERDICTS:
        print()
        print("STOP: waiting for a human decision.")
        print(f"A team member must read both AI answers, the source and the test result,")
        print(f"then set \"verdict\" in {path} to one of: {', '.join(VERDICTS)}.")
        return 3

    # Step 3: route the decision. These are instructions, not automatic actions.
    print()
    print(f"Human verdict: {verdict}")
    if verdict == "accept":
        print("NEXT: commit the change and evidence -> push the branch -> PR review -> CI.")
        print("      CI green is feedback, not proof. A reviewer still reads the diff.")
    elif verdict == "revise":
        print("NEXT: back to revision. Fix the claim/code/test, re-run the test,")
        print("      update 'observed', clear 'verdict', and run this workflow again.")
    else:  # reject
        print("NEXT: do not use the claim. Record why in the attempt log, and ask the")
        print("      question again or find a better source before changing any code.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
