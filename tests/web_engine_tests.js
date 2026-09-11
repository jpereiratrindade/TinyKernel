/**
 * Automated test suite for TinyKernel Web Causal Engine
 * Validates TK-O v0.2.0 epistemological contracts, 5-phase pipeline,
 * evidence typing, dimensional adjudication, and claim inference.
 */

const fs = require('fs');
const path = require('path');

// Load engine.js in sandbox
const engineCode = fs.readFileSync(path.join(__dirname, '../web/js/engine.js'), 'utf8');
const vm = require('vm');
const context = {
  console: console,
  setTimeout: setTimeout,
  TextEncoder: require('util').TextEncoder,
  Uint8Array: Uint8Array,
  Uint32Array: Uint32Array,
  Math: Math,
  Date: Date,
  Array: Array,
  Map: Map,
  Set: Set,
  JSON: JSON
};
vm.createContext(context);
vm.runInContext(engineCode + '; globalThis.TkEngine = TkEngine;', context);

function assert(condition, message) {
  if (!condition) {
    console.error(`FAIL: ${message}`);
    process.exit(1);
  }
}

async function runTests() {
  console.log('--- Starting Web Causal Engine Tests ---');
  const engine = new context.TkEngine();

  // Investigation Desk projections must be read-only and epistemically safe.
  const deskStudy = await engine.buildTkSait001();
  const deskSnapshot = JSON.stringify(deskStudy);
  const workflow = engine.analyzeWorkflow(deskStudy);
  assert(workflow.action.type === 'observe', 'workflow recommends observing an untested baseline');
  assert(workflow.action.blockers.length === 5, 'workflow exposes all missing baseline witnesses');
  const ranked = engine.rankInterventions(deskStudy);
  assert(ranked.length === 6, 'workflow ranks all planned interventions');
  const preview = engine.previewIntervention(deskStudy, ranked[0].intervention);
  assert(preview && preview.before.length === preview.after.length + 1, 'counterfactual preview removes one component');
  const explanation = engine.explainClaim(deskStudy, deskStudy.claims[0]);
  assert(explanation && explanation.next_blocker, 'claim debugger exposes the next epistemic blocker');
  assert(JSON.stringify(deskStudy) === deskSnapshot, 'desk analysis does not mutate scientific state');
  console.log('PASS: investigation_desk_read_only_projections');

  const canonicalDeskStudy = await engine.buildTk0001();
  assert(canonicalDeskStudy.evidence.every(e => e.evidence_type === 'EMPIRICAL_OBSERVATION'), 'canonical runs expose empirical evidence consistently');
  assert(engine.analyzeWorkflow(canonicalDeskStudy).action.type === 'materialize', 'completed canonical study recommends exploring its open frontier');

  // Test 1: TK-SAIT-001 Baseline Rigor
  const sait = await engine.buildTkSait001();
  assert(sait.investigation.id === 'TK-SAIT-001', 'SAIT investigation ID');
  assert(sait.investigation.status === 'formulated', 'SAIT starts formulated');
  assert(sait.realizations.length === 1, 'SAIT has 1 baseline realization');
  assert(sait.interventions.length === 6, 'SAIT has 6 planned interventions');
  assert(sait.evidence.length === 1, 'SAIT has 1 structural record');
  assert(sait.evidence[0].evidence_type === 'STRUCTURAL_RECORD', 'evidence is STRUCTURAL_RECORD');
  assert(sait.claims.every(c => c.status === 'open'), 'all claims open at formulation');
  console.log('PASS: sait_formulation_rigor');

  // Test 2: Phase Separation (Formulated -> Materialized -> Observed -> Adjudicated -> Inferred)
  // Step 2.1: Materialize intervention
  let study = JSON.parse(JSON.stringify(sait));
  study = await engine.applyIntervention(study, {
    kind: 'remove',
    target_component: 'circuitos_curtos_feiras'
  });
  assert(study.investigation.status === 'materialized', 'status materialized after intervention assembly');
  assert(study.realizations.length === 2, '2 realizations exist');
  assert(study.evidence.length === 2, '2 structural records exist');
  assert(study.evidence[1].evidence_type === 'STRUCTURAL_RECORD', 'assembly is STRUCTURAL_RECORD');
  console.log('PASS: phase_materialized');

  // Step 2.2: Observe baseline empirical traces
  for (const w of study.witnesses) {
    study = await engine.injectEmpiricalObservation(study, study.realizations[0].id, w.kind, true, 'trace=measured');
  }
  // Observe broken causal trace on intervention
  study = await engine.injectEmpiricalObservation(study, study.realizations[1].id, 'causal', false, 'trace=broken_causal');
  assert(study.investigation.status === 'observed', 'status observed after observations');
  console.log('PASS: phase_observed');

  // Step 2.3: Adjudicate witnesses
  study = await engine.adjudicateWitnesses(study);
  assert(study.investigation.status === 'adjudicated', 'status adjudicated after adjudication');
  assert(study.adjudications.length === 2, '2 adjudications created');
  assert(study.adjudications[0].classification === 'PRESERVED', 'baseline PRESERVED');
  assert(study.adjudications[1].classification === 'BROKEN_CAUSAL', 'intervention BROKEN_CAUSAL');
  assert(study.adjudications[1].run_id.includes('INT_1') || study.adjudications[1].run_id.includes('R_INT'), 'adjudication points to actual empirical run_id');
  // CRITICAL GATE: Claims must NOT be supported yet!
  assert(study.claims.every(c => c.status === 'open'), 'claims remain OPEN after adjudicateWitnesses');
  console.log('PASS: phase_adjudicated_claims_open');

  // Step 2.4: Infer claims
  study = await engine.inferClaims(study);
  assert(study.investigation.status === 'inferred', 'status inferred after inferClaims');
  const suffClaim = study.claims.find(c => c.id.includes(':Q:SUFFICIENCY'));
  const necClaim = study.claims.find(c => c.id.includes('CIRCUITOS_CURTOS_FEIRAS_NECESSITY'));
  assert(suffClaim && suffClaim.status === 'supported', 'L2 sufficiency SUPPORTED');
  assert(necClaim && necClaim.status === 'supported', 'L3 necessity SUPPORTED');
  const minClaim = study.claims.find(c => c.id.includes(':Q:RELATIVE_MINIMALITY'));
  assert(minClaim && minClaim.status === 'open', 'L5 minimality remains OPEN');
  console.log('PASS: phase_inferred_claims_supported');

  // Test 3: Strict Causal Rupture Gate (WITNESS_COMPROMISED does not support L3)
  let flawedStudy = JSON.parse(JSON.stringify(sait));
  flawedStudy = await engine.applyIntervention(flawedStudy, { kind: 'remove', target_component: 'solo_vivo' });
  for (const w of flawedStudy.witnesses) {
    flawedStudy = await engine.injectEmpiricalObservation(flawedStudy, flawedStudy.realizations[0].id, w.kind, true);
  }
  // Observational apparatus fail instead of causal
  flawedStudy = await engine.injectEmpiricalObservation(flawedStudy, flawedStudy.realizations[1].id, 'observational', false);
  flawedStudy = await engine.adjudicateWitnesses(flawedStudy);
  assert(flawedStudy.adjudications.some(a => a.classification === 'WITNESS_COMPROMISED'), 'classified as WITNESS_COMPROMISED');
  flawedStudy = await engine.inferClaims(flawedStudy);
  const soloNecClaim = flawedStudy.claims.find(c => c.id.includes('SOLO_VIVO_NECESSITY'));
  assert(!soloNecClaim || soloNecClaim.status === 'open', 'WITNESS_COMPROMISED did not support L3 claim');
  // Test 4: Partial Observations produce PARTIALLY_OBSERVED and cannot support L2/L3
  let partialStudy = JSON.parse(JSON.stringify(sait));
  // Observe only 2 of 5 witnesses on baseline
  await engine.injectEmpiricalObservation(partialStudy, partialStudy.realizations[0].id, 'operational', true);
  await engine.injectEmpiricalObservation(partialStudy, partialStudy.realizations[0].id, 'causal', true);
  await engine.adjudicateWitnesses(partialStudy);
  assert(partialStudy.adjudications.length === 1, '1 baseline adjudication produced');
  assert(partialStudy.adjudications[0].classification === 'PARTIALLY_OBSERVED', 'classified as PARTIALLY_OBSERVED');
  assert(partialStudy.adjudications[0].outcome === 'undetermined', 'outcome is undetermined');
  await engine.inferClaims(partialStudy);
  const partialSuff = partialStudy.claims.find(c => c.id.includes(':Q:SUFFICIENCY'));
  assert(partialSuff.status === 'open', 'L2 remains open under partial observations');
  console.log('PASS: partial_observations_inconclusive');

  // Test 5: Monotonic Phase Progression Machine
  let phaseStudy = JSON.parse(JSON.stringify(sait));
  assert(phaseStudy.investigation.status === 'formulated', 'starts formulated');
  engine.advancePhase(phaseStudy.investigation, 'materialized');
  assert(phaseStudy.investigation.status === 'materialized', 'advanced to materialized');
  engine.advancePhase(phaseStudy.investigation, 'observed');
  assert(phaseStudy.investigation.status === 'observed', 'advanced to observed');
  // Attempt to regress back to formulated
  engine.advancePhase(phaseStudy.investigation, 'formulated');
  assert(phaseStudy.investigation.status === 'observed', 'cannot regress phase backwards');
  console.log('PASS: monotonic_phase_progression');

  // Test 7: Photosynthesis Benchmark (Materialize Planned PSII Intervention -> Observe -> Adjudicate -> Infer L3)
  const photoConfig = {
    id: "TK-PHOTO-001",
    phenomenonName: "Fotossíntese Oxigênica",
    phenomenonDesc: "Conversão de energia luminosa em energia química e fixação de CO2",
    contextDesc: "Cloroplastos de plantas C3 sob irradiância saturante",
    dimensions: ["oxigênio", "elétrons", "atp_nadph", "carboidratos", "estabilidade_redox"],
    essentialRelations: ["luz->elétrons->gradiente_prótons->ATP"],
    temporalBounds: ["ciclo_segundos_minutos"],
    baselineLabel: "Aparato Fotossintético Completo",
    baselineComponents: ["PSII", "Cyt_b6f", "PSI", "ATP_Synthase", "Rubisco"],
    initialInterventions: [
      { kind: "remove", target_component: "PSII", prediction: "BROKEN_CAUSAL" },
      { kind: "remove", target_component: "PSI", prediction: "BROKEN_CAUSAL" },
      { kind: "remove", target_component: "Rubisco", prediction: "BROKEN_CAUSAL" },
      { kind: "remove", target_component: "Cyt_b6f", prediction: "BROKEN_CAUSAL" }
    ]
  };

  let photoStudy = await engine.createCustomInvestigation(photoConfig);
  assert(photoStudy.investigation.id === "TK-PHOTO-001", "photo study ID");
  assert(photoStudy.realizations.length === 1, "photo starts with 1 baseline realization");
  assert(photoStudy.interventions.length === 4, "photo starts with 4 planned interventions");
  assert(photoStudy.interventions.every(i => i.status === "planned"), "all 4 interventions are PLANNED");
  assert(photoStudy.claims.length === 6, "6 claims formulated (1 sufficiency + 1 minimality + 4 necessity)");
  assert(photoStudy.claims.every(c => c.status === "open"), "all claims are initially OPEN");

  // Step 7.1: Baseline Empirical Observation & Adjudication
  for (const w of photoStudy.witnesses) {
    photoStudy = await engine.injectEmpiricalObservation(photoStudy, photoStudy.realizations[0].id, w.kind, true, `trace=${w.kind}_measured`);
  }
  photoStudy = await engine.adjudicateWitnesses(photoStudy);
  photoStudy = await engine.inferClaims(photoStudy);
  const photoSuff = photoStudy.claims.find(c => c.id.includes(":Q:SUFFICIENCY"));
  assert(photoSuff && photoSuff.status === "supported", "Baseline sufficiency is SUPPORTED");
  const psiiNecBefore = photoStudy.claims.find(c => c.id.includes("PSII_NECESSITY"));
  assert(psiiNecBefore && psiiNecBefore.status === "open", "PSII necessity is OPEN before materialization");

  // Step 7.2: Materialize Planned Intervention: remove PSII (R0 -> R1)
  const psiiPlannedItv = photoStudy.interventions.find(i => i.target_component === "PSII");
  photoStudy = await engine.applyIntervention(photoStudy, {
    planned_id: psiiPlannedItv.id,
    kind: "remove",
    target_component: "PSII",
    protocol: "inibição DCMU"
  });
  assert(photoStudy.realizations.length === 2, "2 realizations after PSII materialization");
  const r1 = photoStudy.realizations[1];
  assert(!r1.components.includes("PSII"), "R1 components do not include PSII");
  assert(r1.components.includes("PSI") && r1.components.includes("Rubisco"), "R1 preserves other components");
  assert(psiiPlannedItv.status === "performed", "PSII intervention status is now PERFORMED");
  assert(psiiPlannedItv.target === r1.id, "PSII intervention points to R1");

  // Step 7.3: Observe Causal Rupture on R1, Adjudicate & Infer L3
  photoStudy = await engine.injectEmpiricalObservation(photoStudy, r1.id, "causal", false, "trace=no_oxygen_evolution");
  photoStudy = await engine.adjudicateWitnesses(photoStudy);
  const r1Adj = photoStudy.adjudications.find(a => a.run_id && a.run_id.includes("INT_1"));
  assert(r1Adj && r1Adj.classification === "BROKEN_CAUSAL", "R1 adjudicated as BROKEN_CAUSAL");

  photoStudy = await engine.inferClaims(photoStudy);
  const psiiNecAfter = photoStudy.claims.find(c => c.id.includes("PSII_NECESSITY"));
  assert(psiiNecAfter && psiiNecAfter.status === "supported", "PSII necessity is now SUPPORTED via empirical causal chain!");
  console.log('PASS: photosynthesis_materialize_and_l3_inference');

  console.log('\nALL 10 WEB ENGINE EPISTEMIC TESTS PASSED!');
}

runTests().catch(err => {
  console.error('Unhandled error:', err);
  process.exit(1);
});
