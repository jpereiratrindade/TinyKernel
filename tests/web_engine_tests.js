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

  // Test 6: TK-O v0.2.1 Schema Version 2 Compliance
  assert(sait.investigation.schema_version === 2, 'schema version is 2');
  assert(sait.investigation.ontology_version === '0.2.1', 'ontology version is 0.2.1');
  const tk0001 = await engine.buildTk0001();
  assert(tk0001.investigation.schema_version === 2, 'TK-0001 schema version is 2');
  assert(tk0001.investigation.ontology_version === '0.2.1', 'TK-0001 ontology version is 0.2.1');
  console.log('PASS: tko_021_schema_version_2');

  console.log('\nALL 9 WEB ENGINE EPISTEMIC TESTS PASSED!');
}

runTests().catch(err => {
  console.error('Unhandled error:', err);
  process.exit(1);
});
