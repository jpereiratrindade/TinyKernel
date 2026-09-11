#!/usr/bin/env python3
import pathlib
import tempfile
from concurrent.futures import ThreadPoolExecutor

import sys

ROOT = pathlib.Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "web"))

from serve import CoreBridge


with tempfile.TemporaryDirectory(prefix="tinykernel-web-api-") as workspace:
    bridge = CoreBridge(workspace)
    bridge.initialize()
    studies = bridge.list_studies()
    assert studies == ["TK-0000", "TK-0001", "TK-SAIT-001"], studies
    with ThreadPoolExecutor(max_workers=3) as pool:
        concurrent = list(pool.map(bridge.get_study, studies))
    assert [item["investigation"]["id"] for item in concurrent] == studies
    study = bridge.get_study("TK-0001")
    assert study["format"] == "tinykernel-investigation-json"
    assert study["investigation"]["status"] == "inferred"
    assert len(study["runs"]) == 3
    assert all(item["evidence_type"] == "EMPIRICAL_OBSERVATION" for item in study["evidence"])

    custom = bridge.create_study({
        "investigation": {"id": "TK-WEB-001", "title": "Persistência observada"},
        "phenomenon": {"name": "Persistência observada", "description": "Teste transacional web"},
        "context": {"description": "Contexto de teste"},
        "constitutive_profile": {
            "dimensions": ["estado"],
            "essential_relations": ["estado->saída"],
            "temporal_bounds": ["t+1"],
        },
        "realizations": [{"label": "baseline", "components": ["state", "update"]}],
        "interventions": [{"kind": "remove", "target_component": "update", "replacement_component": ""}],
    })
    assert custom["investigation"]["status"] == "formulated"
    planned_id = custom["interventions"][0]["id"]
    custom = bridge.materialize("TK-WEB-001", {
        "planned_id": planned_id,
        "source": "TK-WEB-001:R:BASE",
        "kind": "remove",
        "target_component": "update",
        "protocol": "controlled removal",
    })
    target_id = custom["interventions"][0]["target_realization_id"]
    for dimension in ["operational", "causal", "discriminative", "observational", "temporal"]:
        custom = bridge.observe("TK-WEB-001", {
            "realization_id": "TK-WEB-001:R:BASE", "dimension": dimension,
            "satisfied": True, "trace": f"baseline:{dimension}",
        })
        custom = bridge.observe("TK-WEB-001", {
            "realization_id": target_id, "dimension": dimension,
            "satisfied": dimension != "causal", "trace": f"intervention:{dimension}",
        })
    custom = bridge.adjudicate("TK-WEB-001")
    custom = bridge.infer("TK-WEB-001")
    supported = {claim["level"] for claim in custom["claims"] if claim["status"] == "supported"}
    assert {"L2", "L3"}.issubset(supported)
    evidence_before_revision = len(custom["evidence"])
    revised = bridge.observe("TK-WEB-001", {
        "realization_id": target_id, "dimension": "causal",
        "satisfied": True, "trace": "corrected measurement",
    })
    assert len(revised["evidence"]) == evidence_before_revision + 1
    assert any(":REV:0001" in item["id"] for item in revised["evidence"])
    intervention_run = next(item for item in revised["runs"] if item["result_realization_id"] == target_id)
    stale = next(item for item in revised["adjudications"] if item["run_id"] == intervention_run["id"])
    assert stale["classification"] == "STALE"
    assert not any(claim["status"] == "supported" and claim["level"] == "L3" for claim in revised["claims"])

print("PASS: web API reads canonical studies from libtinykernel")
