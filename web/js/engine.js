/**
 * TinyKernel Web Causal Engine — TK-O v0.2.1
 * Epistemic Separation Architecture:
 * Formulation (DECLARED) -> Structure (MATERIALIZED) -> Observation (OBSERVED) -> Adjudication (ADJUDICATED) -> Inference (INFERRED)
 */

class TkEngine {
  constructor() {
    this.ontologyVersion = "0.2.1";
    this.schemaVersion = 2;
    this.storageKey = "tinykernel_workspace_studies_v2";
    this.apiBase = "/api";
    this.apiAvailable = null;
  }

  // Calculate SHA-256 hash using Web Crypto API or pure fallback
  async sha256(str) {
    if (typeof window !== "undefined" && window.crypto && window.crypto.subtle) {
      const msgBuffer = new TextEncoder().encode(str);
      const hashBuffer = await window.crypto.subtle.digest("SHA-256", msgBuffer);
      const hashArray = Array.from(new Uint8Array(hashBuffer));
      return hashArray.map(b => b.toString(16).padStart(2, "0")).join("");
    }
    return this.sha256SyncFallback(str);
  }

  sha256SyncFallback(input) {
    const K = [
      0x428a2f98,0x71374491,0xb5c0fbcf,0xe9b5dba5,0x3956c25b,0x59f111f1,0x923f82a4,0xab1c5ed5,
      0xd807aa98,0x12835b01,0x243185be,0x550c7dc3,0x72be5d74,0x80deb1fe,0x9bdc06a7,0xc19bf174,
      0xe49b69c1,0xefbe4786,0x0fc19dc6,0x240ca1cc,0x2de92c6f,0x4a7484aa,0x5cb0a9dc,0x76f988da,
      0x983e5152,0xa831c66d,0xb00327c8,0xbf597fc7,0xc6e00bf3,0xd5a79147,0x06ca6351,0x14292967,
      0x27b70a85,0x2e1b2138,0x4d2c6dfc,0x53380d13,0x650a7354,0x766a0abb,0x81c2c92e,0x92722c85,
      0xa2bfe8a1,0xa81a664b,0xc24b8b70,0xc76c51a3,0xd192e819,0xd6990624,0xf40e3585,0x106aa070,
      0x19a4c116,0x1e376c08,0x2748774c,0x34b0bcb5,0x391c0cb3,0x4ed8aa4a,0x5b9cca4f,0x682e6ff3,
      0x748f82ee,0x78a5636f,0x84c87814,0x8cc70208,0x90befffa,0xa4506ceb,0xbef9a3f7,0xc67178f2
    ];

    const bytes = [];
    for (let i = 0; i < input.length; i++) {
      const code = input.charCodeAt(i);
      if (code < 0x80) bytes.push(code);
      else if (code < 0x800) {
        bytes.push(0xc0 | (code >> 6), 0x80 | (code & 0x3f));
      } else if (code < 0xd800 || code >= 0xe000) {
        bytes.push(0xe0 | (code >> 12), 0x80 | ((code >> 6) & 0x3f), 0x80 | (code & 0x3f));
      }
    }

    const bitLength = bytes.length * 8;
    bytes.push(0x80);
    while ((bytes.length % 64) !== 56) bytes.push(0);

    for (let shift = 56; shift >= 0; shift -= 8) {
      bytes.push(Math.floor(bitLength / Math.pow(2, shift)) & 0xff);
    }

    let [a, b, c, d, e, f, g, h] = [
      0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
      0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19
    ];

    const rotr = (n, x) => (x >>> n) | (x << (32 - n));

    for (let offset = 0; offset < bytes.length; offset += 64) {
      const words = new Uint32Array(64);
      for (let i = 0; i < 16; i++) {
        const p = offset + i * 4;
        words[i] = ((bytes[p] << 24) | (bytes[p + 1] << 16) | (bytes[p + 2] << 8) | bytes[p + 3]) >>> 0;
      }
      for (let i = 16; i < 64; i++) {
        const s0 = (rotr(7, words[i - 15]) ^ rotr(18, words[i - 15]) ^ (words[i - 15] >>> 3)) >>> 0;
        const s1 = (rotr(17, words[i - 2]) ^ rotr(19, words[i - 2]) ^ (words[i - 2] >>> 10)) >>> 0;
        words[i] = (words[i - 16] + s0 + words[i - 7] + s1) >>> 0;
      }

      let [A, B, C, D, E, F, G, H] = [a, b, c, d, e, f, g, h];
      for (let i = 0; i < 64; i++) {
        const s1 = (rotr(6, E) ^ rotr(11, E) ^ rotr(25, E)) >>> 0;
        const choice = ((E & F) ^ (~E & G)) >>> 0;
        const temp1 = (H + s1 + choice + K[i] + words[i]) >>> 0;
        const s0 = (rotr(2, A) ^ rotr(13, A) ^ rotr(22, A)) >>> 0;
        const maj = ((A & B) ^ (A & C) ^ (B & C)) >>> 0;
        const temp2 = (s0 + maj) >>> 0;

        H = G; G = F; F = E; E = (D + temp1) >>> 0;
        D = C; C = B; B = A; A = (temp1 + temp2) >>> 0;
      }

      a = (a + A) >>> 0; b = (b + B) >>> 0; c = (c + C) >>> 0; d = (d + D) >>> 0;
      e = (e + E) >>> 0; f = (f + F) >>> 0; g = (g + G) >>> 0; h = (h + H) >>> 0;
    }

    return [a, b, c, d, e, f, g, h].map(x => x.toString(16).padStart(8, "0")).join("");
  }

  witnesses(prefix) {
    return [
      { id: `${prefix}:W:OPERATIONAL`, kind: "operational", description: "O sistema produz comportamento observável." },
      { id: `${prefix}:W:CAUSAL`, kind: "causal", description: "A saída depende da relação causal preregistrada." },
      { id: `${prefix}:W:DISCRIMINATIVE`, kind: "discriminative", description: "As distinções do perfil permanecem separáveis." },
      { id: `${prefix}:W:OBSERVATIONAL`, kind: "observational", description: "O aparato permanece capaz de observar." },
      { id: `${prefix}:W:TEMPORAL`, kind: "temporal", description: "A restrição temporal preregistrada é satisfeita." }
    ];
  }

  // Canonical Reference: TK-0001 (In-Memory Adaptive Persistence Reference)
  async buildTk0001() {
    const study = {
      investigation: {
        id: "TK-0001",
        schema_version: 2,
        ontology_version: "0.2.1",
        title: "Persistência adaptativa",
        phenomenon_id: "TK-0001:P",
        context_id: "TK-0001:C",
        profile_id: "TK-0001:PHI",
        order_declaration: "Gamma=active_causal_relations",
        status: "executed",
        category: "canonical_reference",
        created_at: "2026-09-09T00:00:00-03:00"
      },
      phenomenon: {
        id: "TK-0001:P",
        schema_version: 2,
        ontology_version: "0.2.1",
        name: "persistência adaptativa",
        description: "Uma experiência altera estado persistido e comportamento posterior sem reapresentação da experiência."
      },
      context: {
        id: "TK-0001:C",
        schema_version: 2,
        ontology_version: "0.2.1",
        description: "Execução local determinística, processo único, inteiros binários."
      },
      constitutive_profile: {
        id: "TK-0001:PHI",
        schema_version: 2,
        ontology_version: "0.2.1",
        dimensions: ["estado antes/depois", "experiência presente/ausente", "comportamento baseline/posterior"],
        essential_relations: ["experiência->alteração persistida", "alteração persistida->comportamento posterior"],
        temporal_bounds: ["alteração após experiência", "persistência até interação posterior sem experiência"]
      },
      witnesses: this.witnesses("TK-0001"),
      realizations: [
        {
          id: "TK-0001:R:BASE",
          schema_version: 2,
          ontology_version: "0.2.1",
          investigation_id: "TK-0001",
          label: "baseline adaptativo",
          components: ["state", "input", "action", "feedback", "difference", "update", "persistence", "later_interaction"],
          complexity: 5,
          outcome: "preserving",
          x: 24,
          y: 72
        },
        {
          id: "TK-0001:R:ALT_FEEDBACK",
          schema_version: 2,
          ontology_version: "0.2.1",
          investigation_id: "TK-0001",
          label: "feedback equivalente",
          components: ["state", "input", "action", "feedback_equivalent", "difference", "update", "persistence", "later_interaction"],
          complexity: 5,
          outcome: "preserving",
          x: 320,
          y: 20
        },
        {
          id: "TK-0001:R:NO_UPDATE",
          schema_version: 2,
          ontology_version: "0.2.1",
          investigation_id: "TK-0001",
          label: "sem atualização",
          components: ["state", "input", "action", "feedback", "difference", "persistence", "later_interaction"],
          complexity: 4,
          outcome: "ruptured",
          x: 320,
          y: 124
        }
      ],
      interventions: [
        {
          id: "TK-0001:I:REPLACE_FEEDBACK",
          schema_version: 2,
          ontology_version: "0.2.1",
          investigation_id: "TK-0001",
          kind: "replace",
          source: "TK-0001:R:BASE",
          target: "TK-0001:R:ALT_FEEDBACK",
          target_component: "feedback",
          replacement_component: "feedback_equivalent",
          prediction: "PRESERVED",
          status: "performed",
          x: 200,
          y: 40
        },
        {
          id: "TK-0001:I:REMOVE_UPDATE",
          schema_version: 2,
          ontology_version: "0.2.1",
          investigation_id: "TK-0001",
          kind: "remove",
          source: "TK-0001:R:BASE",
          target: "TK-0001:R:NO_UPDATE",
          target_component: "update",
          replacement_component: "",
          prediction: "BROKEN_CAUSAL",
          status: "performed",
          x: 200,
          y: 140
        },
        {
          id: "TK-0001:I:DISABLE_PERSISTENCE",
          schema_version: 2,
          ontology_version: "0.2.1",
          investigation_id: "TK-0001",
          kind: "disable",
          source: "TK-0001:R:BASE",
          target: "TK-0001:R:DISABLED_PERSISTENCE",
          target_component: "persistence",
          replacement_component: "",
          prediction: "unexecuted",
          status: "planned"
        },
        {
          id: "TK-0001:I:MERGE_STATE_ACTION",
          schema_version: 2,
          ontology_version: "0.2.1",
          investigation_id: "TK-0001",
          kind: "merge",
          source: "TK-0001:R:BASE",
          target: "TK-0001:R:MERGED_STATE_ACTION",
          target_component: "state",
          replacement_component: "action",
          prediction: "unexecuted",
          status: "planned"
        },
        {
          id: "TK-0001:I:PERTURB_FEEDBACK",
          schema_version: 2,
          ontology_version: "0.2.1",
          investigation_id: "TK-0001",
          kind: "perturb",
          source: "TK-0001:R:BASE",
          target: "TK-0001:R:NOISY_FEEDBACK",
          target_component: "feedback",
          replacement_component: "noise",
          prediction: "unexecuted",
          status: "planned"
        }
      ],
      runs: [],
      observations: [],
      evidence: [],
      adjudications: [],
      claims: [
        {
          id: "TK-0001:Q:SUFFICIENCY",
          schema_version: 2,
          ontology_version: "0.2.1",
          subject: "TK-0001:R:BASE",
          assertion: "A realização baseline é suficiente sob o protocolo preregistrado.",
          phenomenon_id: "TK-0001:P",
          context_id: "TK-0001:C",
          level: "L2",
          status: "supported",
          limitations: "Limitado ao contexto, perfil, witnesses e realização TK-0001 declarados.",
          provenance_id: "TK-0001:PROV",
          intervention_scope: [],
          witness_scope: ["TK-0001:W:OPERATIONAL", "TK-0001:W:CAUSAL", "TK-0001:W:DISCRIMINATIVE", "TK-0001:W:OBSERVATIONAL", "TK-0001:W:TEMPORAL"]
        },
        {
          id: "TK-0001:Q:UPDATE_NECESSITY",
          schema_version: 2,
          ontology_version: "0.2.1",
          subject: "update",
          assertion: "A relação de atualização possui necessidade relativa nesta realização.",
          phenomenon_id: "TK-0001:P",
          context_id: "TK-0001:C",
          level: "L3",
          status: "supported",
          limitations: "Não transfere necessidade a outras realizações, contextos ou granularidades.",
          provenance_id: "TK-0001:PROV",
          intervention_scope: ["TK-0001:I:REMOVE_UPDATE"],
          witness_scope: ["TK-0001:W:CAUSAL", "TK-0001:W:TEMPORAL"]
        },
        {
          id: "TK-0001:Q:RELATIVE_MINIMALITY",
          schema_version: 2,
          ontology_version: "0.2.1",
          subject: "TK-0001:R:BASE",
          assertion: "A realização é minimal na ordem Gamma declarada.",
          phenomenon_id: "TK-0001:P",
          context_id: "TK-0001:C",
          level: "L5",
          status: "open",
          limitations: "Espaço incompleto: reduções planejadas e realizações alternativas permanecem abertas.",
          provenance_id: "TK-0001:PROV",
          intervention_scope: [],
          witness_scope: []
        }
      ],
      provenance: [
        {
          id: "TK-0001:PROV",
          schema_version: 2,
          ontology_version: "0.2.1",
          source: "preregistration",
          method: "deterministic built-in adapter",
          timestamp: "2026-09-09T00:00:00-03:00",
          detail: "Materialização TK-SYS-00 derivada de TK-FND-00 v0.2.1."
        }
      ]
    };

    const runConfigs = [
      { suffix: "BASELINE", realization: study.realizations[0], intervention: null, adaptive: true },
      { suffix: "REMOVE_UPDATE", realization: study.realizations[2], intervention: study.interventions[1], adaptive: false },
      { suffix: "REPLACE_FEEDBACK", realization: study.realizations[1], intervention: study.interventions[0], adaptive: true }
    ];

    for (const cfg of runConfigs) {
      const runId = `TK-0001:RUN:${cfg.suffix}`;
      study.runs.push({
        id: runId,
        schema_version: 2,
        ontology_version: "0.2.1",
        investigation_id: "TK-0001",
        intervention_id: cfg.intervention ? cfg.intervention.id : null,
        source_realization_id: cfg.intervention ? cfg.intervention.source : cfg.realization.id,
        target_realization_id: cfg.realization.id,
        status: "completed"
      });

      const obsArtifact = `baseline_action=0;experience_target=1;state_after=${cfg.adaptive ? "1" : "0"};later_action=${cfg.adaptive ? "1" : "0"};experience_represented_later=false`;
      const evidenceIds = [];

      for (const witness of study.witnesses) {
        let satisfied = true;
        if (witness.kind === "causal" || witness.kind === "temporal") satisfied = cfg.adaptive;

        const obsId = `${runId}:O:${witness.kind}`;
        study.observations.push({
          id: obsId,
          schema_version: 2,
          ontology_version: "0.2.1",
          run_id: runId,
          realization_id: cfg.realization.id,
          witness_id: witness.id,
          witness_kind: witness.kind,
          outcome: satisfied ? "satisfied" : "not_satisfied",
          satisfied: satisfied
        });

        const artifact = `run=${runId}\nrealization=${cfg.realization.id}\ndimension=${witness.kind}\nsatisfied=${satisfied ? "true" : "false"}\ntrace=${obsArtifact}\n`;
        const sha256 = await this.sha256(artifact);
        const evidenceId = `${runId}:E:${witness.kind}`;

        study.evidence.push({
          id: evidenceId,
          schema_version: 2,
          ontology_version: "0.2.1",
          run_id: runId,
          witness_id: witness.id,
          observation_ids: [obsId],
          artifact: artifact,
          sha256: sha256,
          evidence_type: "EMPIRICAL_OBSERVATION"
        });
        evidenceIds.push(evidenceId);
      }

      const isPreserved = cfg.adaptive;
      study.adjudications.push({
        id: `${runId}:A`,
        schema_version: 2,
        ontology_version: "0.2.1",
        run_id: runId,
        outcome: isPreserved ? "preserving" : "ruptured",
        classification: isPreserved ? "PRESERVED" : "BROKEN_CAUSAL",
        rule: "TK-O-0.2.1:all-constitutive-dimensions-v1",
        rationale: isPreserved
          ? "Todos os witnesses constitutivos preregistrados foram satisfeitos."
          : "Ao menos uma dimensão constitutiva preregistrada não foi satisfeita.",
        evidence_references: evidenceIds
      });
    }

    return study;
  }

  // Canonical Reference: TK-0000 (Apparatus Bootstrap)
  async buildTk0000() {
    const study = {
      investigation: {
        id: "TK-0000",
        schema_version: 2,
        ontology_version: "0.2.1",
        title: "Bootstrap do aparato experimental",
        phenomenon_id: "TK-0000:P",
        context_id: "TK-0000:C",
        profile_id: "TK-0000:PHI",
        order_declaration: "Gamma=active_causal_relations",
        status: "executed",
        category: "sanity_check",
        created_at: "2026-09-09T00:00:00-03:00"
      },
      phenomenon: {
        id: "TK-0000:P",
        schema_version: 2,
        ontology_version: "0.2.1",
        name: "sentinela operacional",
        description: "Presença observável de um marcador sentinela para verificar o aparato."
      },
      context: {
        id: "TK-0000:C",
        schema_version: 2,
        ontology_version: "0.2.1",
        description: "Execução local determinística, processo único, inteiros binários."
      },
      constitutive_profile: {
        id: "TK-0000:PHI",
        schema_version: 2,
        ontology_version: "0.2.1",
        dimensions: ["sentinela presente/ausente"],
        essential_relations: ["sentinela determina witness"],
        temporal_bounds: ["observação após intervenção"]
      },
      witnesses: this.witnesses("TK-0000"),
      realizations: [
        {
          id: "TK-0000:R:BASE",
          schema_version: 2,
          ontology_version: "0.2.1",
          investigation_id: "TK-0000",
          label: "baseline sentinela",
          components: ["sentinel"],
          complexity: 1,
          outcome: "preserving",
          x: 60,
          y: 80
        },
        {
          id: "TK-0000:R:NO_SENTINEL",
          schema_version: 2,
          ontology_version: "0.2.1",
          investigation_id: "TK-0000",
          label: "sem sentinela",
          components: [],
          complexity: 0,
          outcome: "ruptured",
          x: 340,
          y: 80
        }
      ],
      interventions: [
        {
          id: "TK-0000:I:REMOVE_SENTINEL",
          schema_version: 2,
          ontology_version: "0.2.1",
          investigation_id: "TK-0000",
          kind: "remove",
          source: "TK-0000:R:BASE",
          target: "TK-0000:R:NO_SENTINEL",
          target_component: "sentinel",
          replacement_component: "",
          prediction: "BROKEN_CAUSAL",
          status: "performed",
          x: 200,
          y: 80
        }
      ],
      runs: [],
      observations: [],
      evidence: [],
      adjudications: [],
      claims: [
        {
          id: "TK-0000:Q:APPARATUS",
          schema_version: 2,
          ontology_version: "0.2.1",
          subject: "apparatus",
          assertion: "O aparato observa e discrimina a presença do sentinela com fidelidade determinística.",
          phenomenon_id: "TK-0000:P",
          context_id: "TK-0000:C",
          level: "L1",
          status: "supported",
          limitations: "Válido exclusivamente como sanity check do aparato.",
          provenance_id: "TK-0000:PROV",
          intervention_scope: ["TK-0000:I:REMOVE_SENTINEL"],
          witness_scope: ["TK-0000:W:OPERATIONAL", "TK-0000:W:CAUSAL", "TK-0000:W:DISCRIMINATIVE", "TK-0000:W:OBSERVATIONAL", "TK-0000:W:TEMPORAL"]
        }
      ],
      provenance: [
        {
          id: "TK-0000:PROV",
          schema_version: 2,
          ontology_version: "0.2.1",
          source: "preregistration",
          method: "deterministic built-in adapter",
          timestamp: "2026-09-09T00:00:00-03:00",
          detail: "Sanity check do aparato experimental."
        }
      ]
    };

    const runConfigs = [
      { suffix: "BASELINE", realization: study.realizations[0], intervention: null, marker: true },
      { suffix: "REMOVE_SENTINEL", realization: study.realizations[1], intervention: study.interventions[0], marker: false }
    ];

    for (const cfg of runConfigs) {
      const runId = `TK-0000:RUN:${cfg.suffix}`;
      study.runs.push({
        id: runId,
        schema_version: 2,
        ontology_version: "0.2.1",
        investigation_id: "TK-0000",
        intervention_id: cfg.intervention ? cfg.intervention.id : null,
        source_realization_id: cfg.intervention ? cfg.intervention.source : cfg.realization.id,
        target_realization_id: cfg.realization.id,
        status: "completed"
      });

      const obsArtifact = `sentinel_present=${cfg.marker ? "true" : "false"}`;
      const evidenceIds = [];

      for (const witness of study.witnesses) {
        let satisfied = true;
        if (witness.kind !== "discriminative" && witness.kind !== "observational") {
          satisfied = cfg.marker;
        }

        const obsId = `${runId}:O:${witness.kind}`;
        study.observations.push({
          id: obsId,
          schema_version: 2,
          ontology_version: "0.2.1",
          run_id: runId,
          realization_id: cfg.realization.id,
          witness_id: witness.id,
          witness_kind: witness.kind,
          outcome: satisfied ? "satisfied" : "not_satisfied",
          satisfied: satisfied
        });

        const artifact = `run=${runId}\nrealization=${cfg.realization.id}\ndimension=${witness.kind}\nsatisfied=${satisfied ? "true" : "false"}\ntrace=${obsArtifact}\n`;
        const sha256 = await this.sha256(artifact);
        const evidenceId = `${runId}:E:${witness.kind}`;

        study.evidence.push({
          id: evidenceId,
          schema_version: 2,
          ontology_version: "0.2.1",
          run_id: runId,
          witness_id: witness.id,
          observation_ids: [obsId],
          artifact: artifact,
          sha256: sha256,
          evidence_type: "EMPIRICAL_OBSERVATION"
        });
        evidenceIds.push(evidenceId);
      }

      const isPreserved = cfg.marker;
      study.adjudications.push({
        id: `${runId}:A`,
        schema_version: 2,
        ontology_version: "0.2.1",
        run_id: runId,
        outcome: isPreserved ? "preserving" : "ruptured",
        classification: isPreserved ? "PRESERVED" : "BROKEN_CAUSAL",
        rule: "TK-O-0.2.1:all-constitutive-dimensions-v1",
        rationale: isPreserved
          ? "Todos os witnesses constitutivos preregistrados foram satisfeitos."
          : "Ao menos uma dimensão constitutiva preregistrada não foi satisfeita.",
        evidence_references: evidenceIds
      });
    }

    return study;
  }

  // Foundational Empirical Benchmark: TK-SAIT-001 (Territorial Agroecological System)
  // Rigorously Formulated, 0 Empirical Evidence, Claims OPEN
  async buildTkSait001() {
    const studyId = "TK-SAIT-001";
    const comps = [
      "solo_vivo", "agrobiodiversidade", "reflorestamento_ciliar",
      "armazenamento_hidrico", "sementes_locais", "circuitos_curtos_feiras"
    ];
    
    const study = {
      investigation: {
        id: studyId,
        schema_version: 2,
        ontology_version: "0.2.1",
        title: "Resiliência do Sistema Agroalimentar Territorial (SAIT)",
        phenomenon_id: `${studyId}:P`,
        context_id: `${studyId}:C`,
        profile_id: `${studyId}:PHI`,
        order_declaration: "Gamma=complexidade_constitutiva_decrescente",
        status: "formulated",
        category: "foundational_benchmark",
        created_at: "2026-09-10T11:00:00-03:00"
      },
      phenomenon: {
        id: `${studyId}:P`,
        schema_version: 2,
        ontology_version: "0.2.1",
        name: "resiliência de sistema agroalimentar territorial",
        description: "Capacidade de manter estabilidade produtiva, nutricional e hídrica sob perturbações climáticas e econômicas."
      },
      context: {
        id: `${studyId}:C`,
        schema_version: 2,
        ontology_version: "0.2.1",
        description: "Território semiárido/agreste, agricultura familiar, chuvas irregulares."
      },
      constitutive_profile: {
        id: `${studyId}:PHI`,
        schema_version: 2,
        ontology_version: "0.2.1",
        dimensions: ["estabilidade nutricional", "segurança hídrica", "autonomia sementes"],
        essential_relations: ["solo_vivo->resiliencia_hidrica", "biodiversidade->segurança_nutricional", "feiras_locais->autonomia_economica"],
        temporal_bounds: ["ciclo_anual_safra", "periodo_estiagem_plurianual"]
      },
      witnesses: this.witnesses(studyId),
      realizations: [
        {
          id: `${studyId}:R:BASE`,
          schema_version: 2,
          ontology_version: "0.2.1",
          investigation_id: studyId,
          label: "sistema agroflorestal completo (SAIT baseline)",
          components: comps,
          complexity: comps.length,
          outcome: "untested",
          isBaseline: true,
          x: 30,
          y: 90
        }
      ],
      interventions: [
        { id: `${studyId}:I:REMOVE_circuitos_curtos_feiras`, kind: "remove", source: `${studyId}:R:BASE`, target_component: "circuitos_curtos_feiras", prediction: "BROKEN_CAUSAL", status: "planned" },
        { id: `${studyId}:I:REMOVE_sementes_locais`, kind: "remove", source: `${studyId}:R:BASE`, target_component: "sementes_locais", prediction: "BROKEN_CAUSAL", status: "planned" },
        { id: `${studyId}:I:REMOVE_armazenamento_hidrico`, kind: "remove", source: `${studyId}:R:BASE`, target_component: "armazenamento_hidrico", prediction: "BROKEN_CAUSAL", status: "planned" },
        { id: `${studyId}:I:REMOVE_agrobiodiversidade`, kind: "remove", source: `${studyId}:R:BASE`, target_component: "agrobiodiversidade", prediction: "BROKEN_CAUSAL", status: "planned" },
        { id: `${studyId}:I:REMOVE_reflorestamento_ciliar`, kind: "remove", source: `${studyId}:R:BASE`, target_component: "reflorestamento_ciliar", prediction: "BROKEN_CAUSAL", status: "planned" },
        { id: `${studyId}:I:REMOVE_solo_vivo`, kind: "remove", source: `${studyId}:R:BASE`, target_component: "solo_vivo", prediction: "BROKEN_CAUSAL", status: "planned" }
      ],
      runs: [],
      observations: [],
      evidence: [],
      adjudications: [],
      claims: [
        {
          id: `${studyId}:Q:SUFFICIENCY`,
          schema_version: 2,
          ontology_version: "0.2.1",
          subject: `${studyId}:R:BASE`,
          assertion: "O arranjo territorial baseline é causalmente suficiente para sustentar o perfil de resiliência.",
          phenomenon_id: `${studyId}:P`,
          context_id: `${studyId}:C`,
          level: "L2",
          status: "open",
          limitations: "Aguardando campanhas empíricas de campo (0/5 witnesses observados).",
          provenance_id: `${studyId}:PROV`,
          intervention_scope: [],
          witness_scope: [`${studyId}:W:OPERATIONAL`, `${studyId}:W:CAUSAL`, `${studyId}:W:DISCRIMINATIVE`, `${studyId}:W:OBSERVATIONAL`, `${studyId}:W:TEMPORAL`]
        },
        {
          id: `${studyId}:Q:RELATIVE_MINIMALITY`,
          schema_version: 2,
          ontology_version: "0.2.1",
          subject: `${studyId}:R:BASE`,
          assertion: "O arranjo é minimal no espaço de reduções agroecológicas sob a ordem Gamma.",
          phenomenon_id: `${studyId}:P`,
          context_id: `${studyId}:C`,
          level: "L5",
          status: "open",
          limitations: "Aguardando testes empíricos das 6 intervenções planejadas.",
          provenance_id: `${studyId}:PROV`,
          intervention_scope: [],
          witness_scope: []
        },
        {
          id: `${studyId}:Q:ROBUSTNESS`,
          schema_version: 2,
          ontology_version: "0.2.1",
          subject: `${studyId}:R:BASE`,
          assertion: "A estabilidade produtiva e hídrica é robusta a variações pluviométricas sazonais.",
          phenomenon_id: `${studyId}:P`,
          context_id: `${studyId}:C`,
          level: "L7",
          status: "open",
          limitations: "Requer observações de campo em safras consecutivas.",
          provenance_id: `${studyId}:PROV`,
          intervention_scope: [],
          witness_scope: []
        }
      ],
      provenance: [
        {
          id: `${studyId}:PROV`,
          schema_version: 2,
          ontology_version: "0.2.1",
          source: "territorial_preregistration",
          method: "agroecology_protocol_v1",
          timestamp: "2026-09-10T00:00:00-03:00",
          detail: "Benchmark de sistema agroalimentar territorial formulado com rigor epistemológico."
        }
      ]
    };

    // Add 1 STRUCTURAL_RECORD evidence (fingerprint of formulation protocol)
    const structArtifact = `investigation=${studyId}\nstatus=formulated\nbaseline=untested\nplanned_interventions=6\nempirical_observations=0\n`;
    const structSha256 = await this.sha256(structArtifact);
    study.evidence.push({
      id: `${studyId}:E:STRUCTURAL_INTEGRITY`,
      schema_version: 2,
      ontology_version: "0.2.1",
      run_id: `${studyId}:RUN:SPECIFICATION`,
      witness_id: `${studyId}:W:OBSERVATIONAL`,
      observation_ids: [],
      artifact: structArtifact,
      sha256: structSha256,
      evidence_type: "STRUCTURAL_RECORD"
    });

    return study;
  }

  async createCustomInvestigation(config) {
    return this.preregisterStudy(config);
  }

  async preregisterStudy(config) {
    if (this.apiAvailable !== true) {
      throw new Error("Modo de visualização: a pré-registração exige o núcleo C++ conectado.");
    }
    const payload = {
      investigation: { id: config.id, title: config.title || config.phenomenonName },
      phenomenon: { name: config.phenomenonName, description: config.phenomenonDesc },
      context: { description: config.contextDesc },
      constitutive_profile: {
        dimensions: config.dimensions,
        essential_relations: config.essentialRelations,
        temporal_bounds: config.temporalBounds
      },
      realizations: [{ label: config.baselineLabel, components: config.baselineComponents }],
      interventions: config.initialInterventions || []
    };
    const persisted = await this.apiRequest("/studies", {
      method: "POST",
      body: JSON.stringify(payload)
    });
    return this.normalizeStudy(persisted);
  }

  // Materialize or apply a structural intervention in the workspace
  async applyIntervention(study, itvCfg) {
    if (this.apiAvailable === true && study.runtime_source === "libtinykernel") {
      return this.mutateCoreStudy(study, "interventions", itvCfg);
    }
    throw new Error("Modo de visualização: materializações exigem o núcleo C++ conectado.");
  }

  // Read-only orchestration layer: tells the interface what is scientifically
  // possible next without inventing observations or bypassing epistemic gates.
  analyzeWorkflow(study) {
    if (study.workflow_projection) return study.workflow_projection;
    return {
      current_phase: "formulated", current_phase_index: 0,
      phases: ["formulated", "materialized", "observed", "adjudicated", "inferred"],
      action: { type: "unavailable", title: "Conecte o núcleo para orientar o experimento", reason: "A demonstração estática não calcula decisões epistemológicas.", blockers: ["núcleo C++ indisponível"] },
      completeness: { baseline: { observed: 0, total: 0, missing: [] }, performed_interventions: 0, planned_interventions: 0 },
      allowed_actions: [], intervention_ranking: [], counterfactual_previews: [], claim_explanations: []
    };
  }

  rankInterventions(study) {
    if (study.workflow_projection) {
      return (study.workflow_projection.intervention_ranking || []).map(item => ({
        ...item,
        intervention: (study.interventions || []).find(intervention => intervention.id === item.intervention_id)
      })).filter(item => item.intervention);
    }
    return [];
  }

  previewIntervention(study, interventionOrId) {
    const interventionId = typeof interventionOrId === "string" ? interventionOrId : interventionOrId?.id;
    const projected = (study.workflow_projection?.counterfactual_previews || [])
      .find(item => item.intervention_id === interventionId);
    if (projected) {
      return {
        ...projected,
        intervention: (study.interventions || []).find(item => item.id === projected.intervention_id),
        source: (study.realizations || []).find(item => item.id === projected.source_id)
      };
    }
    return null;
  }

  explainClaim(study, claimOrId) {
    const claimId = typeof claimOrId === "string" ? claimOrId : claimOrId?.id;
    const projected = (study.workflow_projection?.claim_explanations || [])
      .find(item => item.claim_id === claimId);
    return projected ? { ...projected, claim: (study.claims || []).find(item => item.id === claimId) } : null;
  }

  // Inject real empirical observation trace for a single witness dimension
  async injectEmpiricalObservation(study, param2, param3, param4, param5) {
    let realizationId, dimension, passed, rawTrace;
    
    if (typeof param2 === "string" && typeof param3 === "string") {
      realizationId = param2;
      dimension = param3;
      passed = param4;
      rawTrace = param5;
    } else {
      const opts = (typeof param2 === "object" ? param2 : param3) || {};
      realizationId = opts.realization_id || opts.realizationId || (typeof param2 === "string" ? param2 : null);
      dimension = opts.dimension;
      passed = opts.passed !== undefined ? opts.passed : opts.satisfied;
      rawTrace = opts.trace;
    }
    if (!realizationId || !dimension || typeof passed !== "boolean" ||
        typeof rawTrace !== "string" || !rawTrace.trim()) {
      throw new Error("Observação empírica exige realização, dimensão, resultado explícito e traço medido.");
    }

    if (this.apiAvailable === true && study.runtime_source === "libtinykernel") {
      return this.mutateCoreStudy(study, "observations", {
        realization_id: realizationId,
        dimension,
        satisfied: passed,
        trace: rawTrace
      });
    }
    throw new Error("Modo de visualização: observações exigem o núcleo C++ conectado.");
  }

  // Explicit Adjudication: Evaluates all empirical evidence without promoting claims
  async adjudicateWitnesses(study) {
    if (this.apiAvailable === true && study.runtime_source === "libtinykernel") {
      return this.mutateCoreStudy(study, "adjudicate", {});
    }
    throw new Error("Modo de visualização: adjudicações exigem o núcleo C++ conectado.");
  }

  // Explicit Inference: Evaluates claims from adjudications and promotes supported claims
  async inferClaims(study) {
    if (this.apiAvailable === true && study.runtime_source === "libtinykernel") {
      return this.mutateCoreStudy(study, "infer", {});
    }
    throw new Error("Modo de visualização: inferências exigem o núcleo C++ conectado.");
  }

  // Study normalization helper for robust schema compatibility
  normalizeStudy(st) {
    if (!st || typeof st !== "object") return null;
    if (!st.investigation) {
      st.investigation = {
        id: st.id || "TK-CUSTOM",
        schema_version: 2,
        ontology_version: "0.2.1",
        title: st.title || st.name || "Investigação Customizada",
        status: "executed"
      };
    }

    if (typeof st.phenomenon === "string") {
      st.phenomenon = { id: `${st.investigation.id}:P`, name: st.phenomenon, description: st.phenomenon };
    } else if (!st.phenomenon) {
      st.phenomenon = { id: `${st.investigation.id}:P`, name: st.investigation.title || st.investigation.id, description: "Sem descrição formal." };
    } else {
      if (!st.phenomenon.name) st.phenomenon.name = st.investigation.title || st.investigation.id;
      if (!st.phenomenon.description && st.phenomenon.definition) st.phenomenon.description = st.phenomenon.definition;
    }

    if (typeof st.context === "string") {
      st.context = { id: `${st.investigation.id}:C`, description: st.context };
    } else if (!st.context) {
      st.context = { id: `${st.investigation.id}:C`, description: "Execução local determinística." };
    }

    // Canonical C++ exports and browser projections intentionally use the same
    // ontology with slightly different presentation names. Normalize only at
    // this boundary; preserve the original export fields for round-tripping.
    if (st.phenomenon && !st.phenomenon.description && st.phenomenon.definition) {
      st.phenomenon.description = st.phenomenon.definition;
    }

    st.constitutive_profile = st.constitutive_profile || { dimensions: [], essential_relations: [], temporal_bounds: [] };
    if (!st.constitutive_profile.dimensions) st.constitutive_profile.dimensions = st.constitutive_profile.distinctions || [];
    if (!st.constitutive_profile.essential_relations) st.constitutive_profile.essential_relations = st.constitutive_profile.relations || [];
    if (!st.constitutive_profile.temporal_bounds) st.constitutive_profile.temporal_bounds = st.constitutive_profile.temporal_constraints || [];

    st.realizations = Array.isArray(st.realizations) ? st.realizations : [];
    st.interventions = Array.isArray(st.interventions) ? st.interventions : [];
    st.runs = Array.isArray(st.runs) ? st.runs : [];
    st.observations = Array.isArray(st.observations) ? st.observations : [];
    st.evidence = Array.isArray(st.evidence) ? st.evidence : [];
    st.witnesses = Array.isArray(st.witnesses) ? st.witnesses : [];
    st.claims = Array.isArray(st.claims) ? st.claims : [];
    st.adjudications = Array.isArray(st.adjudications) ? st.adjudications : [];

    for (const realization of st.realizations) {
      if (realization.complexity === undefined) realization.complexity = realization.reduction_rank || (realization.components || []).length;
      if (!realization.outcome) realization.outcome = "untested";
    }
    for (const intervention of st.interventions) {
      const isCoreExport = Boolean(intervention.source_realization_id);
      if (!intervention.source) intervention.source = intervention.source_realization_id;
      if (isCoreExport) {
        if (!intervention.target_component) intervention.target_component = intervention.target;
        intervention.target = intervention.target_realization_id || null;
        if (!intervention.replacement_component) intervention.replacement_component = intervention.replacement || "";
        if (intervention.status === "preregistered" && intervention.target) intervention.status = "performed";
      }
    }
    for (const run of st.runs) {
      if (!run.target_realization_id) run.target_realization_id = run.result_realization_id;
      const adjudication = st.adjudications.find(item => item.run_id === run.id);
      const realization = st.realizations.find(item => item.id === run.target_realization_id);
      if (adjudication && realization) realization.outcome = adjudication.outcome;
    }
    for (const observation of st.observations) {
      if (!observation.witness_kind) observation.witness_kind = observation.dimension;
    }

    return st;
  }

  async getCoreStudies() {
    if (typeof fetch === "undefined" || this.apiAvailable === false) return [];
    try {
      const response = await fetch(`${this.apiBase}/studies`, { headers: { Accept: "application/json" } });
      if (!response.ok) throw new Error(`API ${response.status}`);
      const index = await response.json();
      const studies = await Promise.all((index.studies || []).map(async id => {
        const itemResponse = await fetch(`${this.apiBase}/studies/${encodeURIComponent(id)}`, { headers: { Accept: "application/json" } });
        if (!itemResponse.ok) return null;
        return this.normalizeStudy(await itemResponse.json());
      }));
      this.apiAvailable = true;
      return studies.filter(Boolean);
    } catch (error) {
      this.apiAvailable = false;
      console.warn("Núcleo local indisponível; usando referências embarcadas.", error);
      return [];
    }
  }

  replaceStudy(target, source) {
    for (const key of Object.keys(target)) delete target[key];
    Object.assign(target, this.normalizeStudy(source));
    return target;
  }

  async apiRequest(path, options = {}) {
    const response = await fetch(`${this.apiBase}${path}`, {
      ...options,
      headers: { "Content-Type": "application/json", Accept: "application/json", ...(options.headers || {}) }
    });
    const payload = await response.json().catch(() => ({}));
    if (!response.ok) throw new Error(payload.error || `API ${response.status}`);
    return payload;
  }

  async mutateCoreStudy(study, action, payload) {
    const id = encodeURIComponent(study.investigation.id);
    const updated = await this.apiRequest(`/studies/${id}/${action}`, {
      method: "POST",
      body: JSON.stringify(payload || {})
    });
    return this.replaceStudy(study, updated);
  }

  async persistStudy(study) {
    if (this.apiAvailable === true && study.runtime_source === "libtinykernel") return study;
    throw new Error("Modo de visualização: o núcleo C++ não promove estudos locais a estado canônico.");
  }

  async exportStudy(id) {
    const response = await fetch(`${this.apiBase}/studies/${encodeURIComponent(id)}/export`, {
      headers: { Accept: "application/json" }
    });
    if (!response.ok) throw new Error(`Exportação canônica indisponível (${response.status}).`);
    return response.text();
  }

  async exportWorkspace() {
    const response = await fetch(`${this.apiBase}/workspace/export`, {
      headers: { Accept: "application/vnd.sqlite3" }
    });
    if (!response.ok) throw new Error(`Exportação do workspace indisponível (${response.status}).`);
    return response.blob();
  }

  async importWorkspace(file) {
    const response = await fetch(`${this.apiBase}/workspace/import`, {
      method: "POST",
      headers: { "Content-Type": "application/vnd.sqlite3", Accept: "application/json" },
      body: file
    });
    const payload = await response.json().catch(() => ({}));
    if (!response.ok) throw new Error(payload.error || `Importação rejeitada (${response.status}).`);
    return payload;
  }

  // Workspace Storage Management (Local Repository)
  async getAllStudies() {
    let custom = [];
    const legacyKeys = [
      this.storageKey,
      "tinykernel_studies_v1",
      "tinykernel_studies",
      "tinykernel_workspace",
      "tk_studies"
    ];

    if (typeof localStorage !== "undefined") {
      for (const key of legacyKeys) {
        try {
          const raw = localStorage.getItem(key);
          if (raw) {
            const parsed = JSON.parse(raw);
            if (Array.isArray(parsed)) {
              custom.push(...parsed);
            } else if (parsed && Array.isArray(parsed.studies)) {
              custom.push(...parsed.studies);
            } else if (parsed && typeof parsed === "object") {
              custom.push(parsed);
            }
          }
        } catch (e) {
          console.warn("Falha ao ler chave localStorage:", key, e);
        }
      }
    }

    const defaultTk0000 = await this.buildTk0000();
    const defaultTk0001 = await this.buildTk0001();
    const defaultTkSait001 = await this.buildTkSait001();

    const coreStudies = await this.getCoreStudies();

    const map = new Map();
    map.set("TK-0000", defaultTk0000);
    map.set("TK-0001", defaultTk0001);
    map.set("TK-SAIT-001", defaultTkSait001);

    for (const coreStudy of coreStudies) {
      if (coreStudy?.investigation?.id) map.set(coreStudy.investigation.id, coreStudy);
    }

    for (const rawSt of custom) {
      const st = this.normalizeStudy(rawSt);
      if (st && st.investigation && st.investigation.id) {
        // If the user created a custom investigation, include it in the map
        if (!map.has(st.investigation.id)) {
          map.set(st.investigation.id, st);
        }
      }
    }

    return Array.from(map.values()).map(s => this.normalizeStudy(s));
  }

  async getStudy(id) {
    const studies = await this.getAllStudies();
    return studies.find(s => s.investigation.id === id) || null;
  }

  saveStudy(study) {
    if (study && study.runtime_source === "libtinykernel") return true;
    return false;
  }

  async deleteStudy(id, study = null) {
    if (id === "TK-0000" || id === "TK-0001" || id === "TK-SAIT-001") return;
    if (this.apiAvailable === true && study?.runtime_source === "libtinykernel") {
      await this.apiRequest(`/studies/${encodeURIComponent(id)}`, { method: "DELETE" });
      return;
    }
    throw new Error("Modo de visualização: exclusões exigem o núcleo C++ conectado.");
  }

}

// Global instance & export
if (typeof window !== "undefined") {
  window.tkEngine = new TkEngine();
}
if (typeof module !== "undefined" && module.exports) {
  module.exports = { TkEngine };
}
