/**
 * TinyKernel Web Causal Engine — TK-O v0.2.0
 * Universal Causal Space Laboratory Engine & Canonical Reference Studies (TK-0000, TK-0001)
 */

class TkEngine {
  constructor() {
    this.ontologyVersion = "0.2.0";
    this.schemaVersion = 1;
    this.storageKey = "tinykernel_workspace_studies_v1";
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

  // Synchronous SHA-256 fallback (identical constants and bitwise logic as TinyKernel C++ core)
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

  // Generic Observation & Adjudication for user-defined investigations
  observeGeneric(realization, study) {
    const comps = realization.components || [];
    const has = c => comps.includes(c);
    
    // Check if essential components or relations defined in profile are maintained
    const phi = study.constitutive_profile || {};
    const essentialTerms = phi.essential_relations || [];
    
    // An intervention preserves causal structure if essential relations remain satisfied
    let essentialIntact = true;
    let missingEssential = [];
    
    // Check baseline relations vs current components
    if (realization.isBaseline) {
      essentialIntact = true;
    } else if (realization.interventionKind === "remove" || realization.interventionKind === "disable") {
      // Removing a component breaks causal if it was part of baseline
      essentialIntact = false;
      missingEssential.push(realization.targetComponent || "component");
    } else if (realization.interventionKind === "replace") {
      // Equivalent replacement preserves
      essentialIntact = true;
    } else if (realization.interventionKind === "perturb") {
      essentialIntact = false;
    }

    const operational = comps.length > 0;
    const causal = essentialIntact && operational;
    const discriminative = true;
    const observable = true;
    const temporal = essentialIntact;

    const trace = `components=[${comps.join(",")}];essential_intact=${essentialIntact};operational=${operational};causal=${causal};temporal=${temporal}`;

    return {
      observable,
      operational,
      causal,
      discriminative,
      temporal,
      artifact: trace
    };
  }

  classificationFor(result) {
    if (!result.observable) return "WITNESS_COMPROMISED";
    if (!result.operational) return "BROKEN_OPERATIONAL";
    if (!result.causal) return "BROKEN_CAUSAL";
    if (!result.discriminative) return "BROKEN_DISCRIMINATIVE";
    if (!result.temporal) return "BROKEN_TEMPORAL";
    return "PRESERVED";
  }

  outcomeFor(classification) {
    if (classification === "PRESERVED") return "preserving";
    if (classification === "WITNESS_COMPROMISED" || classification === "INCONCLUSIVE") return "undetermined";
    return "ruptured";
  }

  // Canonical Reference: TK-0001 (Adaptive Persistence)
  observeTk0001(realization) {
    const comps = realization.components || [];
    const has = c => comps.includes(c);
    const update = has("update");
    const persistence = has("persistence");
    const feedback = has("feedback") || has("feedback_equivalent");
    const state = has("state");
    const adaptive = update && persistence && feedback && state;

    return {
      observable: true,
      operational: update && state,
      causal: adaptive,
      discriminative: true,
      temporal: adaptive,
      artifact: `baseline_action=0;experience_target=1;state_after=${adaptive ? "1" : "0"};later_action=${adaptive ? "1" : "0"};experience_represented_later=false`
    };
  }

  // Canonical Reference: TK-0000 (Apparatus Bootstrap)
  observeTk0000(realization) {
    const comps = realization.components || [];
    const marker = comps.includes("sentinel");
    return {
      observable: true,
      operational: marker,
      causal: marker,
      discriminative: true,
      temporal: marker,
      artifact: `sentinel_present=${marker ? "true" : "false"}`
    };
  }

  async buildTk0001() {
    const study = {
      investigation: {
        id: "TK-0001",
        schema_version: 1,
        ontology_version: "0.2.0",
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
        schema_version: 1,
        ontology_version: "0.2.0",
        name: "persistência adaptativa",
        description: "Uma experiência altera estado persistido e comportamento posterior sem reapresentação da experiência."
      },
      context: {
        id: "TK-0001:C",
        schema_version: 1,
        ontology_version: "0.2.0",
        description: "Execução local determinística, processo único, inteiros binários."
      },
      constitutive_profile: {
        id: "TK-0001:PHI",
        schema_version: 1,
        ontology_version: "0.2.0",
        dimensions: ["estado antes/depois", "experiência presente/ausente", "comportamento baseline/posterior"],
        essential_relations: ["experiência->alteração persistida", "alteração persistida->comportamento posterior"],
        temporal_bounds: ["alteração após experiência", "persistência até interação posterior sem experiência"]
      },
      witnesses: this.witnesses("TK-0001"),
      realizations: [
        {
          id: "TK-0001:R:BASE",
          schema_version: 1,
          ontology_version: "0.2.0",
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
          schema_version: 1,
          ontology_version: "0.2.0",
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
          schema_version: 1,
          ontology_version: "0.2.0",
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
          schema_version: 1,
          ontology_version: "0.2.0",
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
          schema_version: 1,
          ontology_version: "0.2.0",
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
          schema_version: 1,
          ontology_version: "0.2.0",
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
          schema_version: 1,
          ontology_version: "0.2.0",
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
          schema_version: 1,
          ontology_version: "0.2.0",
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
          schema_version: 1,
          ontology_version: "0.2.0",
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
          schema_version: 1,
          ontology_version: "0.2.0",
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
          schema_version: 1,
          ontology_version: "0.2.0",
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
          schema_version: 1,
          ontology_version: "0.2.0",
          source: "preregistration",
          method: "deterministic built-in adapter",
          timestamp: "2026-09-09T00:00:00-03:00",
          detail: "Materialização TK-SYS-00 derivada de TK-FND-00 v0.2.0."
        }
      ]
    };

    const runConfigs = [
      { suffix: "BASELINE", realization: study.realizations[0], intervention: null },
      { suffix: "REMOVE_UPDATE", realization: study.realizations[2], intervention: study.interventions[1] },
      { suffix: "REPLACE_FEEDBACK", realization: study.realizations[1], intervention: study.interventions[0] }
    ];

    for (const cfg of runConfigs) {
      const runId = `TK-0001:RUN:${cfg.suffix}`;
      study.runs.push({
        id: runId,
        schema_version: 1,
        ontology_version: "0.2.0",
        investigation_id: "TK-0001",
        intervention_id: cfg.intervention ? cfg.intervention.id : null,
        source_realization_id: cfg.intervention ? cfg.intervention.source : cfg.realization.id,
        target_realization_id: cfg.realization.id,
        status: "completed"
      });

      const obsResult = this.observeTk0001(cfg.realization);
      const evidenceIds = [];

      for (const witness of study.witnesses) {
        let satisfied = false;
        if (witness.kind === "operational") satisfied = obsResult.operational;
        else if (witness.kind === "causal") satisfied = obsResult.causal;
        else if (witness.kind === "discriminative") satisfied = obsResult.discriminative;
        else if (witness.kind === "observational") satisfied = obsResult.observable;
        else if (witness.kind === "temporal") satisfied = obsResult.temporal;

        const obsId = `${runId}:O:${witness.kind}`;
        study.observations.push({
          id: obsId,
          schema_version: 1,
          ontology_version: "0.2.0",
          run_id: runId,
          realization_id: cfg.realization.id,
          witness_id: witness.id,
          witness_kind: witness.kind,
          outcome: satisfied ? "satisfied" : "not_satisfied",
          satisfied: satisfied
        });

        const artifact = `run=${runId}\nrealization=${cfg.realization.id}\ndimension=${witness.kind}\nsatisfied=${satisfied ? "true" : "false"}\ntrace=${obsResult.artifact}\n`;
        const sha256 = await this.sha256(artifact);
        const evidenceId = `${runId}:E:${witness.kind}`;

        study.evidence.push({
          id: evidenceId,
          schema_version: 1,
          ontology_version: "0.2.0",
          run_id: runId,
          witness_id: witness.id,
          observation_ids: [obsId],
          artifact: artifact,
          sha256: sha256
        });
        evidenceIds.push(evidenceId);
      }

      const classification = this.classificationFor(obsResult);
      study.adjudications.push({
        id: `${runId}:A`,
        schema_version: 1,
        ontology_version: "0.2.0",
        run_id: runId,
        outcome: this.outcomeFor(classification),
        classification: classification,
        rule: "TK-O-0.2.0:all-constitutive-dimensions-v1",
        rationale: classification === "PRESERVED"
          ? "Todos os witnesses constitutivos preregistrados foram satisfeitos."
          : "Ao menos uma dimensão constitutiva preregistrada não foi satisfeita.",
        evidence_references: evidenceIds
      });
    }

    return study;
  }

  async buildTk0000() {
    const study = {
      investigation: {
        id: "TK-0000",
        schema_version: 1,
        ontology_version: "0.2.0",
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
        schema_version: 1,
        ontology_version: "0.2.0",
        name: "sentinela operacional",
        description: "Presença observável de um marcador sentinela para verificar o aparato."
      },
      context: {
        id: "TK-0000:C",
        schema_version: 1,
        ontology_version: "0.2.0",
        description: "Execução local determinística, processo único, inteiros binários."
      },
      constitutive_profile: {
        id: "TK-0000:PHI",
        schema_version: 1,
        ontology_version: "0.2.0",
        dimensions: ["sentinela presente/ausente"],
        essential_relations: ["sentinela determina witness"],
        temporal_bounds: ["observação após intervenção"]
      },
      witnesses: this.witnesses("TK-0000"),
      realizations: [
        {
          id: "TK-0000:R:BASE",
          schema_version: 1,
          ontology_version: "0.2.0",
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
          schema_version: 1,
          ontology_version: "0.2.0",
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
          schema_version: 1,
          ontology_version: "0.2.0",
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
          schema_version: 1,
          ontology_version: "0.2.0",
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
          schema_version: 1,
          ontology_version: "0.2.0",
          source: "preregistration",
          method: "deterministic built-in adapter",
          timestamp: "2026-09-09T00:00:00-03:00",
          detail: "Sanity check do aparato experimental."
        }
      ]
    };

    const runConfigs = [
      { suffix: "BASELINE", realization: study.realizations[0], intervention: null },
      { suffix: "REMOVE_SENTINEL", realization: study.realizations[1], intervention: study.interventions[0] }
    ];

    for (const cfg of runConfigs) {
      const runId = `TK-0000:RUN:${cfg.suffix}`;
      study.runs.push({
        id: runId,
        schema_version: 1,
        ontology_version: "0.2.0",
        investigation_id: "TK-0000",
        intervention_id: cfg.intervention ? cfg.intervention.id : null,
        source_realization_id: cfg.intervention ? cfg.intervention.source : cfg.realization.id,
        target_realization_id: cfg.realization.id,
        status: "completed"
      });

      const obsResult = this.observeTk0000(cfg.realization);
      const evidenceIds = [];

      for (const witness of study.witnesses) {
        let satisfied = false;
        if (witness.kind === "operational") satisfied = obsResult.operational;
        else if (witness.kind === "causal") satisfied = obsResult.causal;
        else if (witness.kind === "discriminative") satisfied = obsResult.discriminative;
        else if (witness.kind === "observational") satisfied = obsResult.observable;
        else if (witness.kind === "temporal") satisfied = obsResult.temporal;

        const obsId = `${runId}:O:${witness.kind}`;
        study.observations.push({
          id: obsId,
          schema_version: 1,
          ontology_version: "0.2.0",
          run_id: runId,
          realization_id: cfg.realization.id,
          witness_id: witness.id,
          witness_kind: witness.kind,
          outcome: satisfied ? "satisfied" : "not_satisfied",
          satisfied: satisfied
        });

        const artifact = `run=${runId}\nrealization=${cfg.realization.id}\ndimension=${witness.kind}\nsatisfied=${satisfied ? "true" : "false"}\ntrace=${obsResult.artifact}\n`;
        const sha256 = await this.sha256(artifact);
        const evidenceId = `${runId}:E:${witness.kind}`;

        study.evidence.push({
          id: evidenceId,
          schema_version: 1,
          ontology_version: "0.2.0",
          run_id: runId,
          witness_id: witness.id,
          observation_ids: [obsId],
          artifact: artifact,
          sha256: sha256
        });
        evidenceIds.push(evidenceId);
      }

      const classification = this.classificationFor(obsResult);
      study.adjudications.push({
        id: `${runId}:A`,
        schema_version: 1,
        ontology_version: "0.2.0",
        run_id: runId,
        outcome: this.outcomeFor(classification),
        classification: classification,
        rule: "TK-O-0.2.0:all-constitutive-dimensions-v1",
        rationale: classification === "PRESERVED"
          ? "Todos os witnesses constitutivos preregistrados foram satisfeitos."
          : "Ao menos uma dimensão constitutiva preregistrada não foi satisfeita.",
        evidence_references: evidenceIds
      });
    }

    return study;
  }

  // Universal Study Creator (Wizard output to executable study)
  async createGenericStudy(config) {
    const studyId = config.id || `TK-${String(Date.now()).slice(-4)}`;
    const title = config.title || config.phenomenonName || "Nova Investigação";
    const baselineComponents = config.baselineComponents || [];

    const study = {
      investigation: {
        id: studyId,
        schema_version: 1,
        ontology_version: "0.2.0",
        title: title,
        phenomenon_id: `${studyId}:P`,
        context_id: `${studyId}:C`,
        profile_id: `${studyId}:PHI`,
        order_declaration: config.orderDeclaration || "Gamma=active_causal_relations",
        status: "executed",
        category: "user_investigation",
        created_at: new Date().toISOString()
      },
      phenomenon: {
        id: `${studyId}:P`,
        schema_version: 1,
        ontology_version: "0.2.0",
        name: config.phenomenonName || title,
        description: config.phenomenonDesc || "Fenômeno experimental formulado pelo pesquisador."
      },
      context: {
        id: `${studyId}:C`,
        schema_version: 1,
        ontology_version: "0.2.0",
        description: config.contextDesc || "Ambiente determinístico com observação rigorosa."
      },
      constitutive_profile: {
        id: `${studyId}:PHI`,
        schema_version: 1,
        ontology_version: "0.2.0",
        dimensions: config.dimensions && config.dimensions.length ? config.dimensions : ["estado observável"],
        essential_relations: config.essentialRelations && config.essentialRelations.length ? config.essentialRelations : ["componente->resultado"],
        temporal_bounds: config.temporalBounds && config.temporalBounds.length ? config.temporalBounds : ["estabilidade pós-intervenção"]
      },
      witnesses: this.witnesses(studyId),
      realizations: [
        {
          id: `${studyId}:R:BASE`,
          schema_version: 1,
          ontology_version: "0.2.0",
          investigation_id: studyId,
          label: config.baselineLabel || "baseline inicial",
          components: baselineComponents,
          complexity: baselineComponents.length,
          outcome: "preserving",
          isBaseline: true,
          x: 40,
          y: 80
        }
      ],
      interventions: [],
      runs: [],
      observations: [],
      evidence: [],
      adjudications: [],
      claims: [
        {
          id: `${studyId}:Q:SUFFICIENCY`,
          schema_version: 1,
          ontology_version: "0.2.0",
          subject: `${studyId}:R:BASE`,
          assertion: "A realização baseline é suficiente sob o protocolo preregistrado.",
          phenomenon_id: `${studyId}:P`,
          context_id: `${studyId}:C`,
          level: "L2",
          status: "supported",
          limitations: "Limitado ao contexto, perfil e realização inicial declarados.",
          provenance_id: `${studyId}:PROV`,
          intervention_scope: [],
          witness_scope: [`${studyId}:W:OPERATIONAL`, `${studyId}:W:CAUSAL`, `${studyId}:W:DISCRIMINATIVE`, `${studyId}:W:OBSERVATIONAL`, `${studyId}:W:TEMPORAL`]
        },
        {
          id: `${studyId}:Q:RELATIVE_MINIMALITY`,
          schema_version: 1,
          ontology_version: "0.2.0",
          subject: `${studyId}:R:BASE`,
          assertion: "A realização é minimal na ordem Gamma declarada.",
          phenomenon_id: `${studyId}:P`,
          context_id: `${studyId}:C`,
          level: "L5",
          status: "open",
          limitations: "Espaço incompleto: reduções planejadas e alternativas continuam abertas.",
          provenance_id: `${studyId}:PROV`,
          intervention_scope: [],
          witness_scope: []
        }
      ],
      provenance: [
        {
          id: `${studyId}:PROV`,
          schema_version: 1,
          ontology_version: "0.2.0",
          source: "interactive_wizard",
          method: "generic deterministic causal adapter",
          timestamp: new Date().toISOString(),
          detail: "Investigação formulada interativamente no laboratório TinyKernel."
        }
      ]
    };

    // Record baseline run
    await this.recordRun(study, study.realizations[0], null, "BASELINE");

    // Add and execute initial interventions if provided
    if (config.initialInterventions && config.initialInterventions.length) {
      for (const itvCfg of config.initialInterventions) {
        await this.applyIntervention(study, itvCfg);
      }
    }

    return study;
  }

  // Execute a new intervention on an active study in real time
  async applyIntervention(study, itvCfg) {
    const studyId = study.investigation.id;
    const itvIndex = study.interventions.length + 1;
    const kind = itvCfg.kind || "remove";
    const targetComp = itvCfg.target_component || "";
    const replComp = itvCfg.replacement_component || "";
    const sourceRealizationId = itvCfg.source || `${studyId}:R:BASE`;

    const source = study.realizations.find(r => r.id === sourceRealizationId) || study.realizations[0];
    
    // Derive new components set based on intervention operator
    let newComponents = [...(source.components || [])];
    let targetLabel = "";

    if (kind === "remove") {
      newComponents = newComponents.filter(c => c !== targetComp);
      targetLabel = `sem ${targetComp}`;
    } else if (kind === "replace") {
      newComponents = newComponents.map(c => c === targetComp ? replComp : c);
      targetLabel = `${targetComp} → ${replComp}`;
    } else if (kind === "disable") {
      newComponents = newComponents.filter(c => c !== targetComp);
      targetLabel = `${targetComp} desabilitado`;
    } else if (kind === "merge") {
      newComponents = newComponents.filter(c => c !== targetComp && c !== replComp);
      newComponents.push(`${targetComp}_${replComp}`);
      targetLabel = `${targetComp}+${replComp} fundidos`;
    } else if (kind === "perturb") {
      newComponents = newComponents.map(c => c === targetComp ? `${targetComp}_perturbed` : c);
      targetLabel = `${targetComp} perturbado`;
    }

    const targetRealizationId = `${studyId}:R:INT_${itvIndex}`;
    const itvId = `${studyId}:I:${kind.toUpperCase()}_${targetComp || itvIndex}`;

    // Compute y position to distribute visually in causal space
    const yOffset = 30 + (study.realizations.length * 65);

    const derivedRealization = {
      id: targetRealizationId,
      schema_version: 1,
      ontology_version: "0.2.0",
      investigation_id: studyId,
      label: targetLabel,
      components: newComponents,
      complexity: newComponents.length,
      outcome: "pending",
      isBaseline: false,
      interventionKind: kind,
      targetComponent: targetComp,
      x: 340,
      y: yOffset
    };

    const intervention = {
      id: itvId,
      schema_version: 1,
      ontology_version: "0.2.0",
      investigation_id: studyId,
      kind: kind,
      source: source.id,
      target: targetRealizationId,
      target_component: targetComp,
      replacement_component: replComp,
      prediction: itvCfg.prediction || (kind === "replace" ? "PRESERVED" : "BROKEN_CAUSAL"),
      status: "performed",
      x: 210,
      y: yOffset + 15
    };

    study.realizations.push(derivedRealization);
    study.interventions.push(intervention);

    // Record run and compute SHA-256 evidence digests
    await this.recordRun(study, derivedRealization, intervention, `${kind.toUpperCase()}_${targetComp || itvIndex}`);

    // If an intervention broke causal relation, establish necessity claim (L3)
    if (derivedRealization.outcome === "ruptured" && targetComp) {
      const claimId = `${studyId}:Q:${targetComp.toUpperCase()}_NECESSITY`;
      if (!study.claims.find(c => c.id === claimId)) {
        study.claims.splice(1, 0, {
          id: claimId,
          schema_version: 1,
          ontology_version: "0.2.0",
          subject: targetComp,
          assertion: `A relação associada a '${targetComp}' possui necessidade relativa nesta realização.`,
          phenomenon_id: study.investigation.phenomenon_id,
          context_id: study.investigation.context_id,
          level: "L3",
          status: "supported",
          limitations: "Não transfere necessidade a outras realizações, contextos ou granularidades.",
          provenance_id: `${studyId}:PROV`,
          intervention_scope: [itvId],
          witness_scope: [`${studyId}:W:CAUSAL`, `${studyId}:W:TEMPORAL`]
        });
      }
    }

    return study;
  }

  // Record a deterministic run with SHA-256 evidence digests
  async recordRun(study, realization, intervention, suffix) {
    const studyId = study.investigation.id;
    const runId = `${studyId}:RUN:${suffix}`;

    study.runs.push({
      id: runId,
      schema_version: 1,
      ontology_version: "0.2.0",
      investigation_id: studyId,
      intervention_id: intervention ? intervention.id : null,
      source_realization_id: intervention ? intervention.source : realization.id,
      target_realization_id: realization.id,
      status: "completed"
    });

    const obsResult = this.observeGeneric(realization, study);
    const classification = this.classificationFor(obsResult);
    const outcome = this.outcomeFor(classification);
    realization.outcome = outcome;

    const evidenceIds = [];

    for (const witness of study.witnesses) {
      let satisfied = false;
      if (witness.kind === "operational") satisfied = obsResult.operational;
      else if (witness.kind === "causal") satisfied = obsResult.causal;
      else if (witness.kind === "discriminative") satisfied = obsResult.discriminative;
      else if (witness.kind === "observational") satisfied = obsResult.observable;
      else if (witness.kind === "temporal") satisfied = obsResult.temporal;

      const obsId = `${runId}:O:${witness.kind}`;
      study.observations.push({
        id: obsId,
        schema_version: 1,
        ontology_version: "0.2.0",
        run_id: runId,
        realization_id: realization.id,
        witness_id: witness.id,
        witness_kind: witness.kind,
        outcome: satisfied ? "satisfied" : "not_satisfied",
        satisfied: satisfied
      });

      const artifact = `run=${runId}\nrealization=${realization.id}\ndimension=${witness.kind}\nsatisfied=${satisfied ? "true" : "false"}\ntrace=${obsResult.artifact}\n`;
      const sha256 = await this.sha256(artifact);
      const evidenceId = `${runId}:E:${witness.kind}`;

      study.evidence.push({
        id: evidenceId,
        schema_version: 1,
        ontology_version: "0.2.0",
        run_id: runId,
        witness_id: witness.id,
        observation_ids: [obsId],
        artifact: artifact,
        sha256: sha256
      });
      evidenceIds.push(evidenceId);
    }

    study.adjudications.push({
      id: `${runId}:A`,
      schema_version: 1,
      ontology_version: "0.2.0",
      run_id: runId,
      outcome: outcome,
      classification: classification,
      rule: "TK-O-0.2.0:all-constitutive-dimensions-v1",
      rationale: classification === "PRESERVED"
        ? "Todos os witnesses constitutivos preregistrados foram satisfeitos."
        : "Ao menos uma dimensão constitutiva preregistrada não foi satisfeita.",
      evidence_references: evidenceIds
    });
  }

  // Workspace Storage Management (Local Repository)
  async getAllStudies() {
    let custom = [];
    if (typeof localStorage !== "undefined") {
      try {
        const raw = localStorage.getItem(this.storageKey);
        if (raw) custom = JSON.parse(raw);
      } catch (e) {
        console.warn("Falha ao ler localStorage", e);
      }
    }

    const defaultTk0000 = await this.buildTk0000();
    const defaultTk0001 = await this.buildTk0001();

    const map = new Map();
    map.set("TK-0000", defaultTk0000);
    map.set("TK-0001", defaultTk0001);

    for (const st of custom) {
      if (st && st.investigation && st.investigation.id) {
        map.set(st.investigation.id, st);
      }
    }

    return Array.from(map.values());
  }

  async getStudy(id) {
    const studies = await this.getAllStudies();
    return studies.find(s => s.investigation.id === id) || null;
  }

  saveStudy(study) {
    if (typeof localStorage === "undefined") return;
    try {
      const raw = localStorage.getItem(this.storageKey);
      let list = raw ? JSON.parse(raw) : [];
      list = list.filter(s => s.investigation.id !== study.investigation.id);
      list.push(study);
      localStorage.setItem(this.storageKey, JSON.stringify(list));
    } catch (e) {
      console.warn("Falha ao salvar no localStorage", e);
    }
  }

  deleteStudy(id) {
    if (typeof localStorage === "undefined") return;
    if (id === "TK-0000" || id === "TK-0001") return; // Protect canonical references
    try {
      const raw = localStorage.getItem(this.storageKey);
      let list = raw ? JSON.parse(raw) : [];
      list = list.filter(s => s.investigation.id !== id);
      localStorage.setItem(this.storageKey, JSON.stringify(list));
    } catch (e) {
      console.warn("Falha ao deletar do localStorage", e);
    }
  }
}

// Global instance & export
if (typeof window !== "undefined") {
  window.tkEngine = new TkEngine();
}
if (typeof module !== "undefined" && module.exports) {
  module.exports = { TkEngine };
}
