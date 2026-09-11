/**
 * Browser-boundary tests. Scientific decisions are projected by libtinykernel;
 * the disconnected JavaScript engine is deliberately read-only.
 */

const fs = require('fs');
const path = require('path');
const vm = require('vm');

const engineCode = fs.readFileSync(path.join(__dirname, '../web/js/engine.js'), 'utf8');
const context = {
  console, setTimeout,
  TextEncoder: require('util').TextEncoder,
  Uint8Array, Uint32Array, Math, Date, Array, Map, Set, JSON
};
vm.createContext(context);
vm.runInContext(engineCode + '; globalThis.TkEngine = TkEngine;', context);

function assert(condition, message) {
  if (!condition) throw new Error(message);
}

async function rejects(operation, message) {
  let rejected = false;
  try { await operation(); } catch (error) { rejected = error.message.includes('núcleo C++'); }
  assert(rejected, message);
}

async function runTests() {
  const engine = new context.TkEngine();
  const study = await engine.buildTkSait001();
  const snapshot = JSON.stringify(study);

  const unavailable = engine.analyzeWorkflow(study);
  assert(unavailable.action.type === 'unavailable', 'offline advisor is unavailable');
  assert(engine.rankInterventions(study).length === 0, 'offline mode does not rank interventions');
  assert(engine.previewIntervention(study, study.interventions[0]) === null, 'offline mode does not predict counterfactuals');
  assert(engine.explainClaim(study, study.claims[0]) === null, 'offline mode does not adjudicate claims');
  assert(JSON.stringify(study) === snapshot, 'viewer projections do not mutate state');

  study.workflow_projection = {
    current_phase: 'formulated', current_phase_index: 0,
    phases: ['formulated', 'materialized', 'observed', 'adjudicated', 'inferred'],
    action: { type: 'observe', title: 'Observe', reason: 'Core decision', blockers: ['causal'] },
    completeness: { baseline: { observed: 0, total: 5, missing: ['causal'] }, performed_interventions: 0, planned_interventions: 6 },
    allowed_actions: ['observe', 'materialize'],
    intervention_ranking: [{ intervention_id: study.interventions[0].id, score: 6, reason: 'Core ranking' }],
    counterfactual_previews: [{ intervention_id: study.interventions[0].id, source_id: study.realizations[0].id,
      before: study.realizations[0].components, after: study.realizations[0].components.slice(1),
      removed: [study.realizations[0].components[0]], added: [], affected_claims: [] }],
    claim_explanations: [{ claim_id: study.claims[0].id, supported: false,
      next_blocker: 'Baseline', steps: [{ label: 'Baseline', passed: false }] }]
  };
  assert(engine.analyzeWorkflow(study) === study.workflow_projection, 'browser renders core workflow projection');
  assert(engine.rankInterventions(study)[0].intervention.id === study.interventions[0].id, 'browser hydrates core ranking');
  assert(engine.previewIntervention(study, study.interventions[0]).source.id === study.realizations[0].id, 'browser hydrates core preview');
  assert(engine.explainClaim(study, study.claims[0]).next_blocker === 'Baseline', 'browser renders core claim explanation');

  await rejects(() => engine.applyIntervention(study, {}), 'offline intervention is rejected');
  let incompleteObservationRejected = false;
  try { await engine.injectEmpiricalObservation(study, {}); }
  catch (error) { incompleteObservationRejected = error.message.includes('traço medido'); }
  assert(incompleteObservationRejected, 'browser does not fabricate empirical observation defaults');
  await rejects(() => engine.injectEmpiricalObservation(study, {
    realization_id: study.realizations[0].id, dimension: 'causal', satisfied: true, trace: 'measured'
  }), 'offline observation is rejected');
  await rejects(() => engine.adjudicateWitnesses(study), 'offline adjudication is rejected');
  await rejects(() => engine.inferClaims(study), 'offline inference is rejected');
  await rejects(() => engine.persistStudy(study), 'offline persistence is rejected');

  assert(study.investigation.status === 'formulated', 'historical SAIT benchmark remains formulated');
  assert(study.evidence[0].evidence_type === 'STRUCTURAL_RECORD', 'fixture preserves structural evidence type');
  console.log('PASS: browser is a read-only renderer of core epistemic projections');
}

runTests().catch(error => {
  console.error(`FAIL: ${error.message}`);
  process.exit(1);
});
