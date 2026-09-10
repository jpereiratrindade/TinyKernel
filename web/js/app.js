/**
 * TinyKernel Web Application Controller (SisTer Style)
 * Full Multi-View Laboratory Architecture: Lab Home -> Wizard TK-000X -> Workbench
 */

document.addEventListener("DOMContentLoaded", async () => {
  const state = {
    currentView: "lab-home", // 'lab-home' | 'wizard' | 'workbench'
    activeStudyId: null,
    activeStudy: null,
    selectedEntity: null,
    activeRunId: null,
    graph: null,
    wizard: {
      currentStep: 1,
      maxSteps: 6,
      data: {
        id: "TK-0002",
        phenomenonName: "",
        phenomenonDesc: "",
        contextDesc: "",
        dimensions: [],
        essentialRelations: [],
        temporalBounds: [],
        baselineLabel: "baseline completa",
        baselineComponents: [],
        initialInterventions: []
      }
    }
  };

  // View Containers
  const viewLabHome = document.getElementById("view-lab-home");
  const viewWizard = document.getElementById("view-wizard");
  const viewWorkbench = document.getElementById("view-workbench");
  const breadcrumbCurrent = document.getElementById("breadcrumb-current");

  // Global Header Elements
  const btnBrandHome = document.getElementById("btn-brand-home");
  const btnNavNewInvestigation = document.getElementById("btn-nav-new-investigation");
  const btnHeroNewInvestigation = document.getElementById("btn-hero-new-investigation");
  const btnHeroOpenCanonical = document.getElementById("btn-hero-open-canonical");
  const btnGlobalImport = document.getElementById("btn-global-import");
  const btnGlobalExport = document.getElementById("btn-global-export");
  const fileImportInput = document.getElementById("file-import-input");

  // Workbench Elements
  const btnWorkbenchBackToLab = document.getElementById("btn-workbench-back-to-lab");
  const btnWorkbenchNewObservation = document.getElementById("btn-workbench-new-observation");
  const btnWorkbenchNewIntervention = document.getElementById("btn-workbench-new-intervention");
  const btnWorkbenchAdjudicate = document.getElementById("btn-workbench-adjudicate");
  const btnWorkbenchInfer = document.getElementById("btn-workbench-infer");
  const btnWorkbenchExport = document.getElementById("btn-workbench-export");
  const workbenchStatusBadge = document.getElementById("workbench-status-badge");

  // Modals
  const modalOverlay = document.getElementById("modal-overlay");
  const modalTitle = document.getElementById("modal-title");
  const modalContent = document.getElementById("modal-content");
  const btnModalClose = document.getElementById("btn-modal-close");

  const modalAddIntervention = document.getElementById("modal-add-intervention");
  const btnCloseModalIntervention = document.getElementById("btn-close-modal-intervention");
  const btnCancelModalIntervention = document.getElementById("btn-cancel-modal-intervention");
  const formAddIntervention = document.getElementById("form-add-intervention");
  const modalItvSource = document.getElementById("modal-itv-source");
  const modalItvKind = document.getElementById("modal-itv-kind");
  const modalItvTarget = document.getElementById("modal-itv-target");
  const modalItvReplacement = document.getElementById("modal-itv-replacement");
  const groupModalItvReplacement = document.getElementById("group-modal-itv-replacement");

  const modalAddObservation = document.getElementById("modal-add-observation");
  const btnCloseModalObservation = document.getElementById("btn-close-modal-observation");
  const btnCancelModalObservation = document.getElementById("btn-cancel-modal-observation");
  const formAddObservation = document.getElementById("form-add-observation");
  const modalObsRealization = document.getElementById("modal-obs-realization");
  const modalObsDimension = document.getElementById("modal-obs-dimension");
  const modalObsStatus = document.getElementById("modal-obs-status");
  const modalObsTrace = document.getElementById("modal-obs-trace");

  // Wizard Elements
  const btnWizPrev = document.getElementById("btn-wiz-prev");
  const btnWizNext = document.getElementById("btn-wiz-next");
  const stepIndicators = document.querySelectorAll(".step-indicator");

  // Initialize SVG Causal Graph
  state.graph = new TkCausalGraph("graph-container", (id, raw) => {
    handleEntitySelection(id, raw);
  });

  // ========================================================
  // ROUTING & VIEW NAVIGATION
  // ========================================================
  function switchView(viewName) {
    state.currentView = viewName;
    viewLabHome.classList.remove("active");
    viewWizard.classList.remove("active");
    viewWorkbench.classList.remove("active");

    if (viewName === "lab-home") {
      viewLabHome.classList.add("active");
      breadcrumbCurrent.textContent = "Laboratório";
      renderLabHome();
    } else if (viewName === "wizard") {
      viewWizard.classList.add("active");
      breadcrumbCurrent.textContent = "Nova Investigação";
      initWizard();
    } else if (viewName === "workbench") {
      viewWorkbench.classList.add("active");
      breadcrumbCurrent.textContent = state.activeStudy ? `${state.activeStudy.investigation.id} — ${state.activeStudy.investigation.title}` : "Workbench";
      renderWorkbench();
    }
  }

  // ========================================================
  // VIEW 1: LAB HOME (CATALOG OF INVESTIGATIONS)
  // ========================================================
  async function renderLabHome() {
    try {
      const studies = await window.tkEngine.getAllStudies();

      // Global Stats
      let totalEvidence = 0;
      let totalClaims = 0;
      studies.forEach(s => {
        totalEvidence += (s.evidence || []).length;
        totalClaims += (s.claims || []).filter(c => c.status === "supported").length;
      });

      const elInv = document.getElementById("global-stat-investigations");
      const elEv = document.getElementById("global-stat-evidence");
      const elCl = document.getElementById("global-stat-claims");
      if (elInv) elInv.textContent = studies.length;
      if (elEv) elEv.textContent = totalEvidence;
      if (elCl) elCl.textContent = totalClaims;

      const grid = document.getElementById("investigations-grid-container");
      if (!grid) return;
      grid.innerHTML = "";

      studies.forEach(study => {
        if (!study || !study.investigation) return;
        const inv = study.investigation;
        const isCanonical = inv.id === "TK-0001";
        const isSanity = inv.id === "TK-0000";
        const isBenchmark = inv.id === "TK-SAIT-001";
        
        let badgeCategory = "user";
        let badgeText = "Investigação Inédita";
        if (isCanonical) {
          badgeCategory = "canonical";
          badgeText = "Canônico • Referência";
        } else if (isSanity) {
          badgeCategory = "sanity";
          badgeText = "Sanity • Bootstrap";
        } else if (isBenchmark) {
          badgeCategory = "benchmark";
          badgeText = "Benchmark Territorial";
        }

        const invStatus = inv.status || "executed";
        const statusClass = invStatus === "formulated" ? "formulated" : (invStatus === "materialized" ? "materialized" : "executed");
        const statusText = invStatus.toUpperCase();

        const title = inv.title || (study.phenomenon && study.phenomenon.name) || inv.id;
        const desc = (study.phenomenon && (study.phenomenon.description || study.phenomenon.definition)) || "Sem descrição";

        const supportedClaims = (study.claims || []).filter(c => c.status === "supported").length;
        const empiricalEvCount = (study.evidence || []).filter(e => e.evidence_type === "EMPIRICAL_OBSERVATION").length;
        const structuralEvCount = (study.evidence || []).filter(e => e.evidence_type === "STRUCTURAL_RECORD" || !e.evidence_type).length;

        const evColor = empiricalEvCount > 0 ? "var(--status-preserved)" : "var(--muted)";

        const card = document.createElement("div");
        card.className = "investigation-card";
        card.innerHTML = `
          <div class="card-top">
            <div style="display: flex; align-items: center; gap: 0.5rem;">
              <span class="card-id">${inv.id}</span>
              <span class="card-status-badge ${statusClass}">${statusText}</span>
            </div>
            <span class="card-category-badge ${badgeCategory}">${badgeText}</span>
          </div>
          <div class="card-title">${title}</div>
          <div class="card-desc">${desc}</div>
          <div class="card-metrics">
            <div class="metric-item">
              <span class="label">Realizações</span>
              <span class="value">${(study.realizations || []).length}</span>
            </div>
            <div class="metric-item">
              <span class="label">Intervenções</span>
              <span class="value">${(study.interventions || []).length}</span>
            </div>
            <div class="metric-item">
              <span class="label">Evidências Emp.</span>
              <span class="value" style="color: ${evColor};">${empiricalEvCount}</span>
            </div>
          </div>
          <div class="card-footer">
            <span>Claims: <strong style="color: var(--status-ready);">${supportedClaims}/${(study.claims || []).length}</strong></span>
            <span style="color: var(--accent); font-weight: 600;">Abrir Workbench &rarr;</span>
          </div>
        `;

        card.addEventListener("click", () => {
          openStudyWorkbench(inv.id);
        });

        grid.appendChild(card);
      });

      // Add New Investigation Card
      const addCard = document.createElement("div");
      addCard.className = "add-investigation-card";
      addCard.innerHTML = `
        <div class="add-icon">+</div>
        <div style="font-weight: 700; font-size: 1rem; color: var(--fg);">Formular Nova Investigação</div>
        <div style="font-size: 0.78rem; text-align: center; max-width: 240px;">
          Defina um novo fenômeno, contexto, perfil constitutivo e realize intervenções causais.
        </div>
      `;
      addCard.addEventListener("click", () => {
        switchView("wizard");
      });
      grid.appendChild(addCard);
    } catch (err) {
      console.error("Erro ao renderizar Lab Home:", err);
    }
  }

  async function openStudyWorkbench(studyId) {
    state.activeStudyId = studyId;
    state.activeStudy = await window.tkEngine.getStudy(studyId);
    state.selectedEntity = null;
    state.activeRunId = state.activeStudy && state.activeStudy.runs && state.activeStudy.runs.length ? state.activeStudy.runs[0].id : null;
    switchView("workbench");
  }

  // ========================================================
  // VIEW 2: WIZARD CONTROLLER
  // ========================================================
  function initWizard() {
    state.wizard.currentStep = 1;
    // Auto increment ID based on existing count
    window.tkEngine.getAllStudies().then(all => {
      const nextNum = all.length;
      document.getElementById("wiz-id").value = `TK-${String(nextNum).padStart(4, "0")}`;
    });

    // Clear tag builders
    renderTagList("wiz-dim-tags", state.wizard.data.dimensions, removeDimension);
    renderTagList("wiz-rel-tags", state.wizard.data.essentialRelations, removeRelation);
    renderTagList("wiz-temp-tags", state.wizard.data.temporalBounds, removeTemporal);
    renderTagList("wiz-comp-tags", state.wizard.data.baselineComponents, removeComponent);
    renderInterventionList();

    updateWizardSteps();
  }

  function updateWizardSteps() {
    stepIndicators.forEach(ind => {
      const step = parseInt(ind.getAttribute("data-step"));
      ind.classList.remove("active", "done");
      if (step === state.wizard.currentStep) ind.classList.add("active");
      else if (step < state.wizard.currentStep) ind.classList.add("done");
    });

    for (let i = 1; i <= state.wizard.maxSteps; i++) {
      const pane = document.getElementById(`wizard-pane-${i}`);
      if (pane) {
        if (i === state.wizard.currentStep) pane.classList.add("active");
        else pane.classList.remove("active");
      }
    }

    btnWizPrev.disabled = state.wizard.currentStep === 1;
    btnWizNext.textContent = state.wizard.currentStep === state.wizard.maxSteps ? "Executar & Abrir Investigação" : "Próximo →";
  }

  function renderTagList(containerId, list, onRemove) {
    const container = document.getElementById(containerId);
    container.innerHTML = "";
    list.forEach((item, index) => {
      const pill = document.createElement("span");
      pill.className = "tag-pill";
      pill.innerHTML = `<span>${item}</span><button type="button">&times;</button>`;
      pill.querySelector("button").addEventListener("click", () => onRemove(index));
      container.appendChild(pill);
    });
  }

  function removeDimension(index) {
    state.wizard.data.dimensions.splice(index, 1);
    renderTagList("wiz-dim-tags", state.wizard.data.dimensions, removeDimension);
  }
  function removeRelation(index) {
    state.wizard.data.essentialRelations.splice(index, 1);
    renderTagList("wiz-rel-tags", state.wizard.data.essentialRelations, removeRelation);
  }
  function removeTemporal(index) {
    state.wizard.data.temporalBounds.splice(index, 1);
    renderTagList("wiz-temp-tags", state.wizard.data.temporalBounds, removeTemporal);
  }
  function removeComponent(index) {
    state.wizard.data.baselineComponents.splice(index, 1);
    renderTagList("wiz-comp-tags", state.wizard.data.baselineComponents, removeComponent);
  }

  function renderInterventionList() {
    const listContainer = document.getElementById("wiz-itv-list");
    listContainer.innerHTML = "";
    state.wizard.data.initialInterventions.forEach((itv, index) => {
      const pill = document.createElement("span");
      pill.className = "tag-pill";
      pill.innerHTML = `<span><strong>${itv.kind}</strong>: ${itv.target_component}</span><button type="button">&times;</button>`;
      pill.querySelector("button").addEventListener("click", () => {
        state.wizard.data.initialInterventions.splice(index, 1);
        renderInterventionList();
      });
      listContainer.appendChild(pill);
    });
  }

  // Tag inputs on Enter key
  function setupTagInput(inputId, onAdd) {
    const input = document.getElementById(inputId);
    input.addEventListener("keydown", (e) => {
      if (e.key === "Enter") {
        e.preventDefault();
        const val = input.value.trim();
        if (val) {
          onAdd(val);
          input.value = "";
        }
      }
    });
  }

  setupTagInput("wiz-dim-input", (val) => {
    state.wizard.data.dimensions.push(val);
    renderTagList("wiz-dim-tags", state.wizard.data.dimensions, removeDimension);
  });
  setupTagInput("wiz-rel-input", (val) => {
    state.wizard.data.essentialRelations.push(val);
    renderTagList("wiz-rel-tags", state.wizard.data.essentialRelations, removeRelation);
  });
  setupTagInput("wiz-temp-input", (val) => {
    state.wizard.data.temporalBounds.push(val);
    renderTagList("wiz-temp-tags", state.wizard.data.temporalBounds, removeTemporal);
  });
  setupTagInput("wiz-comp-input", (val) => {
    state.wizard.data.baselineComponents.push(val);
    renderTagList("wiz-comp-tags", state.wizard.data.baselineComponents, removeComponent);
  });

  document.getElementById("btn-wiz-add-itv").addEventListener("click", () => {
    const kind = document.getElementById("wiz-itv-kind").value;
    const target = document.getElementById("wiz-itv-target").value.trim();
    if (!target) return;
    state.wizard.data.initialInterventions.push({
      kind,
      target_component: target
    });
    document.getElementById("wiz-itv-target").value = "";
    renderInterventionList();
  });

  btnWizPrev.addEventListener("click", () => {
    if (state.wizard.currentStep > 1) {
      state.wizard.currentStep--;
      updateWizardSteps();
    }
  });

  btnWizNext.addEventListener("click", async () => {
    if (state.wizard.currentStep < state.wizard.maxSteps) {
      state.wizard.currentStep++;
      updateWizardSteps();
    } else {
      // Step 6 completed: Create & Instantiate Study
      btnWizNext.disabled = true;
      btnWizNext.textContent = "Materializando & Executando...";

      const data = state.wizard.data;
      data.id = document.getElementById("wiz-id").value.trim() || `TK-${Date.now().toString().slice(-4)}`;
      data.phenomenonName = document.getElementById("wiz-phenom-name").value.trim() || data.id;
      data.phenomenonDesc = document.getElementById("wiz-phenom-desc").value.trim() || "Fenômeno experimental formulado pelo pesquisador.";
      data.contextDesc = document.getElementById("wiz-context-desc").value.trim() || "Execução local determinística, processo único, inteiros binários.";
      data.baselineLabel = document.getElementById("wiz-baseline-label").value.trim() || "baseline completa";

      // If user provided no components, provide defaults
      if (!data.baselineComponents.length) {
        data.baselineComponents = ["sensor", "integrator", "threshold", "actuator"];
      }

      const newStudy = await window.tkEngine.createGenericStudy(data);
      window.tkEngine.saveStudy(newStudy);

      btnWizNext.disabled = false;
      openStudyWorkbench(newStudy.investigation.id);
    }
  });

  // ========================================================
  // VIEW 3: WORKBENCH CONTROLLER
  // ========================================================
  function renderWorkbench() {
    if (!state.activeStudy) return;
    const s = state.activeStudy;

    document.getElementById("workbench-investigation-title").textContent = `${s.investigation.id} — ${s.investigation.title || s.phenomenon.name}`;
    
    // Status Badge
    const invStatus = s.investigation.status || "executed";
    workbenchStatusBadge.className = `card-status-badge ${invStatus}`;
    workbenchStatusBadge.textContent = invStatus.toUpperCase();

    renderStats();
    renderPhenomenon();
    renderGraph();
    renderFrontierDock();
    renderRuns();
    renderClaims();
    renderFrontierAnalysis();
    renderInspector(state.selectedEntity ? state.selectedEntity.id : null, state.selectedEntity ? state.selectedEntity.data : null);
  }

  function renderStats() {
    const s = state.activeStudy;
    document.getElementById("stat-realizations").textContent = s.realizations.length;
    document.getElementById("stat-interventions").textContent = s.interventions.length;
    
    // Separate structural integrity records from empirical field observations
    const empiricalEvCount = (s.evidence || []).filter(e => e.evidence_type === "EMPIRICAL_OBSERVATION").length;
    const structuralEvCount = (s.evidence || []).filter(e => e.evidence_type === "STRUCTURAL_RECORD" || !e.evidence_type).length;

    document.getElementById("stat-structural-records").textContent = structuralEvCount;
    document.getElementById("stat-empirical-evidence").textContent = empiricalEvCount;

    const supportedClaims = (s.claims || []).filter(c => c.status === "supported").length;
    document.getElementById("stat-claims").textContent = `${supportedClaims}/${(s.claims || []).length}`;

    const executedItvs = (s.interventions || []).filter(i => i.status === "performed").length;
    const plannedItvs = (s.interventions || []).length - executedItvs;
    document.getElementById("stat-interventions-caption").textContent = `${executedItvs} executadas • ${plannedItvs} planejadas`;

    const untestedRuns = (s.runs || []).filter(r => r.status === "untested" || r.status === "formulated").length;
    document.getElementById("stat-realizations-caption").textContent = `${s.realizations.length} no espaço (${untestedRuns} não testadas)`;
  }

  function renderPhenomenon() {
    const s = state.activeStudy;
    document.getElementById("phenom-title").textContent = s.phenomenon.name || s.investigation.title;
    document.getElementById("phenom-desc").textContent = s.phenomenon.description;
    document.getElementById("context-desc").textContent = s.context.description;

    const profileBody = document.getElementById("profile-dimensions-body");
    profileBody.innerHTML = "";

    const phi = s.constitutive_profile;
    const dims = phi.dimensions || [];
    const rels = phi.essential_relations || [];
    const temps = phi.temporal_bounds || [];

    const maxRows = Math.max(dims.length, rels.length, temps.length);
    for (let i = 0; i < maxRows; i++) {
      const tr = document.createElement("tr");
      tr.innerHTML = `
        <td class="dim-name">${dims[i] || "—"}</td>
        <td>${rels[i] || "—"}</td>
        <td>${temps[i] || "—"}</td>
      `;
      profileBody.appendChild(tr);
    }
  }

  function renderGraph() {
    state.graph.setData(state.activeStudy.realizations, state.activeStudy.interventions);
  }

  function renderFrontierDock() {
    const s = state.activeStudy;
    const executedItvs = s.interventions.filter(i => i.status === "performed").length;
    const plannedItvs = s.interventions.length - executedItvs;
    const untestedRuns = s.runs.filter(r => r.status === "untested" || r.status === "formulated").length;

    document.getElementById("frontier-known-realizations").textContent = s.realizations.length;
    document.getElementById("frontier-unexplored-interventions").textContent = plannedItvs;
    document.getElementById("frontier-open-questions").textContent = (s.claims || []).filter(c => c.status === "open").length;
    document.getElementById("frontier-status-text").textContent = plannedItvs > 0 || untestedRuns > 0
      ? "Espaço Incompleto (Incompleteness by Design)"
      : "Espaço Causalmente Adjudicado";
  }

  function renderRuns() {
    const s = state.activeStudy;
    const list = document.getElementById("runs-list");
    list.innerHTML = "";

    if (!s.runs.length) {
      list.innerHTML = `<div style="color: var(--muted); font-size: 0.8rem; padding: 0.5rem 0;">Nenhuma execução materializada ainda.</div>`;
      return;
    }

    s.runs.forEach(run => {
      const item = document.createElement("div");
      item.className = `run-item ${state.activeRunId === run.id ? 'active' : ''}`;
      
      const adj = s.adjudications.find(a => a.run_id === run.id);
      const outcome = adj ? adj.outcome : "untested";
      const statusClass = outcome === "preserving" ? "preserved" : (outcome === "ruptured" ? "ruptured" : "untested");
      const statusText = outcome ? outcome.toUpperCase() : "UNTESTED";

      const evs = s.evidence.filter(e => e.run_id === run.id);

      item.innerHTML = `
        <div style="font-weight: 700; color: var(--fg); font-size: 0.82rem;">${run.id.split(":").slice(2).join(":") || run.id}</div>
        <div style="font-size: 0.72rem; color: var(--muted); margin-top: 0.2rem;">
          Alvo: <code>${run.target_realization_id ? run.target_realization_id.split(":").slice(2).join(":") : '—'}</code>
        </div>
        <div style="display: flex; align-items: center; justify-content: space-between; margin-top: 0.35rem;">
          <span class="run-status-pill ${statusClass}">${statusText}</span>
          <span style="font-size: 0.72rem; color: var(--accent);">${evs.length} evidências</span>
        </div>
      `;

      item.addEventListener("click", () => {
        state.activeRunId = run.id;
        renderRuns();
        renderInspector(null, null);
      });

      list.appendChild(item);
    });
  }

  function renderClaims() {
    const s = state.activeStudy;
    const list = document.getElementById("claims-list");
    list.innerHTML = "";

    if (!s.claims || !s.claims.length) {
      list.innerHTML = `<div style="color: var(--muted); font-size: 0.8rem; padding: 0.5rem 0;">Nenhum claim formulado.</div>`;
      return;
    }

    s.claims.forEach(claim => {
      const item = document.createElement("div");
      item.className = "claim-card";

      const isSupported = claim.status === "supported";
      const statusText = isSupported ? "SUPPORTED" : "OPEN";

      item.innerHTML = `
        <div class="claim-header">
          <div style="display: flex; align-items: center; gap: 0.4rem;">
            <span class="claim-level-pill">${claim.level}</span>
            <strong style="color: var(--fg); font-size: 0.82rem;">${claim.id.split(":").slice(2).join(":") || claim.id}</strong>
          </div>
          <span class="claim-status ${isSupported ? 'supported' : 'open'}">${statusText}</span>
        </div>
        <div style="font-size: 0.78rem; color: var(--fg); margin: 0.3rem 0; line-height: 1.35;">
          ${claim.assertion}
        </div>
        <div class="claim-footer">
          <span>Sujeito: <code>${claim.subject}</code></span>
          <span>Evidências: <strong>${(claim.evidence_references || []).length}</strong></span>
        </div>
      `;

      item.addEventListener("click", () => {
        handleEntitySelection(claim.id, claim);
      });

      list.appendChild(item);
    });
  }

  function renderFrontierAnalysis() {
    const s = state.activeStudy;
    const preserving = s.realizations.filter(r => r.outcome === "preserving");
    const ruptured = s.realizations.filter(r => r.outcome === "ruptured");
    const untested = s.realizations.filter(r => !r.outcome || r.outcome === "untested" || r.outcome === "partially_observed");

    const minimalCand = preserving.length ? preserving.reduce((min, r) => r.components.length < min.components.length ? r : min, preserving[0]) : null;

    document.getElementById("frontier-analysis-text").innerHTML = `
      <div style="line-height: 1.5;">
        <div>• Realizações Conhecidas: <strong>${s.realizations.length}</strong> (${preserving.length} preservadoras, ${ruptured.length} rompidas, ${untested.length} não totalmente observadas)</div>
        <div>• Realização Minimal Atual: <strong style="color: var(--status-preserved);">${minimalCand ? `${minimalCand.id} (|Σ|=${minimalCand.components.length})` : 'Nenhuma comprovada'}</strong></div>
        <div>• Intervenções Abertas: <strong>${s.interventions.filter(i => i.status !== 'performed').length}</strong> planejadas</div>
        <div style="margin-top: 0.35rem; color: #94a3b8; font-style: italic;">
          Nota Epistêmica: O espaço causal é incompleto por design. Conclusões causais aplicam-se estritamente às intervenções preregistradas e evidências empíricas coletadas.
        </div>
      </div>
    `;
  }

  function renderInspector(id, entity) {
    const container = document.getElementById("inspector-details-container");
    const s = state.activeStudy;

    if (!id || !entity) {
      const run = s.runs.find(r => r.id === state.activeRunId) || s.runs[0];
      const adj = s.adjudications.find(a => a.run_id === (run ? run.id : ""));
      const evs = s.evidence.filter(e => e.run_id === (run ? run.id : ""));

      container.innerHTML = `
        <div class="inspector-card">
          <h4>Inspeção de Execução: ${run ? run.id : "Nenhuma"}</h4>
          <div><strong>Status da Run:</strong> <span class="run-status-pill ${run && (run.status === 'untested' || run.status === 'formulated' || run.status === 'in_progress') ? 'untested' : 'preserved'}">${run ? (run.status || 'completed').toUpperCase() : '—'}</span></div>
          <div><strong>Classificação Causal:</strong> ${adj ? adj.classification : "UNTESTED (Aguardando observação)"}</div>
          <div><strong>Regra Adjudicada:</strong> <code>${adj ? adj.rule : "—"}</code></div>
          <div><strong>Justificativa:</strong> ${adj ? adj.rationale : "Nenhuma adjudicação realizada."}</div>
          <div style="margin-top: 0.5rem;"><strong>Registros & Evidências Vinculadas (${evs.length}):</strong></div>
          ${evs.length === 0 ? `<div style="color: var(--muted); font-size: 0.75rem; margin-top: 0.25rem;">Nenhuma evidência empírica vinculada a esta execução específica. (Registros estruturais no workspace: ${s.evidence.filter(e => e.evidence_type === 'STRUCTURAL_RECORD').length}). Use <em>+ Registrar Observação Empírica</em> para coletar evidências.</div>` : ''}
          ${evs.map(ev => {
            const isEmpirical = ev.evidence_type === "EMPIRICAL_OBSERVATION";
            const badgeType = isEmpirical ? "empirical" : "structural";
            const badgeLabel = isEmpirical ? "Evidência Empírica" : "Registro Estrutural";
            return `
              <div style="margin-top: 0.4rem; padding: 0.35rem; background: rgba(15, 23, 42, 0.6); border-radius: 0.35rem; border: 1px solid rgba(148, 163, 184, 0.15);">
                <div style="display: flex; align-items: center; justify-content: space-between; margin-bottom: 0.2rem;">
                  <span style="color: #94a3b8; font-size: 0.72rem;">${ev.id.split(":").slice(2).join(":")}</span>
                  <span class="evidence-badge ${badgeType}">${badgeLabel}</span>
                </div>
                <div class="hash-preview">${ev.sha256}</div>
                ${ev.artifact ? `<pre style="font-size: 0.68rem; color: #cbd5e1; margin-top: 0.25rem; white-space: pre-wrap;">${ev.artifact}</pre>` : ''}
              </div>
            `;
          }).join("")}
        </div>
      `;
      return;
    }

    let detailsHtml = "";
    if (entity.level) {
      detailsHtml = `
        <div class="inspector-card">
          <h4>Claim: ${entity.id}</h4>
          <div><strong>Nível:</strong> <span class="claim-level-pill">${entity.level}</span></div>
          <div><strong>Sujeito:</strong> ${entity.subject}</div>
          <div><strong>Afirmação:</strong> ${entity.assertion}</div>
          <div><strong>Status:</strong> <span class="claim-status ${entity.status === 'supported' ? 'supported' : 'open'}">${entity.status.toUpperCase()}</span></div>
          <div><strong>Limitações:</strong> ${entity.limitations || "Nenhuma declarada"}</div>
          <div style="margin-top: 0.4rem;"><strong>Testemunhas / Escopo:</strong> ${(entity.witness_scope || []).join(", ") || "Nenhuma"}</div>
        </div>
      `;
    } else if (entity.kind) {
      detailsHtml = `
        <div class="inspector-card">
          <h4>Intervenção: ${entity.id}</h4>
          <div><strong>Tipo:</strong> ${entity.kind}</div>
          <div><strong>Origem:</strong> ${entity.source || entity.source_realization_id}</div>
          <div><strong>Destino:</strong> ${entity.target || entity.target_realization_id || "Aberto"}</div>
          <div><strong>Componente Alvo:</strong> <code>${entity.target_component || "—"}</code></div>
          <div><strong>Substituição:</strong> <code>${entity.replacement_component || "—"}</code></div>
          <div><strong>Predição:</strong> ${entity.prediction}</div>
          <div><strong>Status:</strong> <span class="run-status-pill ${entity.status === 'performed' ? 'preserved' : 'planned'}">${entity.status.toUpperCase()}</span></div>
        </div>
      `;
    } else if (entity.components) {
      detailsHtml = `
        <div class="inspector-card">
          <h4>Realização: ${entity.id}</h4>
          <div><strong>Rótulo:</strong> ${entity.label}</div>
          <div><strong>Complexidade (|&Sigma;|):</strong> ${entity.components.length}</div>
          <div><strong>Resultado Causal:</strong> <span style="color: ${entity.outcome === "preserving" ? "#34d399" : (entity.outcome === "ruptured" ? "#f87171" : "#94a3b8")}; font-weight: 700;">${(entity.outcome || "UNTESTED").toUpperCase()}</span></div>
          <div style="margin-top: 0.4rem;"><strong>Componentes Declarados:</strong></div>
          <div style="display: flex; flex-wrap: wrap; gap: 0.35rem; margin-top: 0.25rem;">
            ${entity.components.map(c => `<span class="sister-tag">${c}</span>`).join("")}
          </div>
        </div>
      `;
    } else {
      detailsHtml = `
        <div class="inspector-card">
          <h4>Entidade: ${id}</h4>
          <pre class="json-view">${JSON.stringify(entity, null, 2)}</pre>
        </div>
      `;
    }

    container.innerHTML = detailsHtml;
  }

  function handleEntitySelection(id, entity) {
    state.selectedEntity = { id, data: entity };
    renderInspector(id, entity);
  }

  // ========================================================
  // MODAL: ADICIONAR INTERVENÇÃO DINÂMICA
  // ========================================================
  btnWorkbenchNewIntervention.addEventListener("click", () => {
    if (!state.activeStudy) return;
    const s = state.activeStudy;

    modalItvSource.innerHTML = "";
    s.realizations.forEach(r => {
      const opt = document.createElement("option");
      opt.value = r.id;
      opt.textContent = `${r.id} (${r.label}) [|Σ|=${r.components.length}]`;
      modalItvSource.appendChild(opt);
    });

    modalItvTarget.value = "";
    modalItvReplacement.value = "";
    modalAddIntervention.classList.add("active");
  });

  modalItvKind.addEventListener("change", (e) => {
    const kind = e.target.value;
    if (kind === "replace" || kind === "merge") {
      groupModalItvReplacement.style.display = "flex";
    } else {
      groupModalItvReplacement.style.display = "none";
    }
  });

  formAddIntervention.addEventListener("submit", async (e) => {
    e.preventDefault();
    const sourceId = modalItvSource.value;
    const kind = modalItvKind.value;
    const targetComp = modalItvTarget.value.trim();
    const replComp = modalItvReplacement.value.trim();

    if (!targetComp) return;

    await window.tkEngine.applyIntervention(state.activeStudy, {
      source: sourceId,
      kind: kind,
      target_component: targetComp,
      replacement_component: replComp
    });

    window.tkEngine.saveStudy(state.activeStudy);
    modalAddIntervention.classList.remove("active");
    renderWorkbench();
  });

  btnCloseModalIntervention.addEventListener("click", () => modalAddIntervention.classList.remove("active"));
  btnCancelModalIntervention.addEventListener("click", () => modalAddIntervention.classList.remove("active"));

  // ========================================================
  // MODAL: REGISTRAR OBSERVAÇÃO EMPÍRICA
  // ========================================================
  btnWorkbenchNewObservation.addEventListener("click", () => {
    if (!state.activeStudy) return;
    const s = state.activeStudy;

    modalObsRealization.innerHTML = "";
    s.realizations.forEach(r => {
      const opt = document.createElement("option");
      opt.value = r.id;
      opt.textContent = `${r.id} (${r.label}) [|Σ|=${r.components.length}]`;
      modalObsRealization.appendChild(opt);
    });

    modalObsTrace.value = "";
    modalAddObservation.classList.add("active");
  });

  formAddObservation.addEventListener("submit", async (e) => {
    e.preventDefault();
    const realizationId = modalObsRealization.value;
    const dimension = modalObsDimension.value;
    const satisfied = modalObsStatus.value === "true";
    const trace = modalObsTrace.value.trim() || `measurement_status=${satisfied}; timestamp=${new Date().toISOString()}`;

    await window.tkEngine.injectEmpiricalObservation(state.activeStudy, {
      realization_id: realizationId,
      dimension: dimension,
      satisfied: satisfied,
      trace: trace
    });

    window.tkEngine.saveStudy(state.activeStudy);
    modalAddObservation.classList.remove("active");
    renderWorkbench();
  });

  btnCloseModalObservation.addEventListener("click", () => modalAddObservation.classList.remove("active"));
  btnCancelModalObservation.addEventListener("click", () => modalAddObservation.classList.remove("active"));

  // ========================================================
  // ADJUDICAR WITNESSES (SEM INFERÊNCIA AUTOMÁTICA)
  // ========================================================
  btnWorkbenchAdjudicate.addEventListener("click", async () => {
    if (!state.activeStudy) return;
    btnWorkbenchAdjudicate.disabled = true;
    btnWorkbenchAdjudicate.textContent = "Adjudicando...";

    await window.tkEngine.adjudicateWitnesses(state.activeStudy);
    window.tkEngine.saveStudy(state.activeStudy);

    const adjCount = (state.activeStudy.adjudications || []).length;
    alert(`Adjudicação concluída! ${adjCount} realização(ões) adjudicada(s). Os claims continuam abertos até a etapa de inferência.`);

    btnWorkbenchAdjudicate.disabled = false;
    btnWorkbenchAdjudicate.textContent = "⚖ Adjudicar Witnesses";
    renderWorkbench();
  });

  // ========================================================
  // INFERIR CLAIMS (AVALIAÇÃO EPISTÊMICA FORMAL)
  // ========================================================
  if (btnWorkbenchInfer) {
    btnWorkbenchInfer.addEventListener("click", async () => {
      if (!state.activeStudy) return;
      btnWorkbenchInfer.disabled = true;
      btnWorkbenchInfer.textContent = "Inferindo...";

      await window.tkEngine.inferClaims(state.activeStudy);
      window.tkEngine.saveStudy(state.activeStudy);

      const supported = state.activeStudy.claims.filter(c => c.status === "supported").length;
      alert(`Inferência concluída! ${supported}/${state.activeStudy.claims.length} claims sustentados formalmente por evidência empírica.`);

      btnWorkbenchInfer.disabled = false;
      btnWorkbenchInfer.textContent = "⚡ Inferir Claims";
      renderWorkbench();
    });
  }

  // ========================================================
  // GLOBAL LISTENERS
  // ========================================================
  btnBrandHome.addEventListener("click", () => switchView("lab-home"));
  btnWorkbenchBackToLab.addEventListener("click", () => switchView("lab-home"));
  btnNavNewInvestigation.addEventListener("click", () => switchView("wizard"));
  btnHeroNewInvestigation.addEventListener("click", () => switchView("wizard"));
  btnHeroOpenCanonical.addEventListener("click", () => openStudyWorkbench("TK-0001"));

  btnWorkbenchExport.addEventListener("click", () => {
    const jsonStr = JSON.stringify(state.activeStudy, null, 2);
    modalTitle.textContent = `Exportação Determinística — ${state.activeStudy.investigation.id}`;
    modalContent.innerHTML = `
      <p style="color: #94a3b8; font-size: 0.82rem;">JSON de exportação determinística conforme TK-O v0.2.0.</p>
      <div style="display: flex; gap: 0.75rem; margin-top: 0.5rem;">
        <button class="btn btn-primary btn-sm" id="btn-copy-json">Copiar JSON</button>
        <button class="btn btn-teal btn-sm" id="btn-download-json">Baixar Arquivo .json</button>
      </div>
      <pre class="json-view">${jsonStr}</pre>
    `;
    modalOverlay.classList.add("active");

    document.getElementById("btn-copy-json").addEventListener("click", () => {
      navigator.clipboard.writeText(jsonStr);
      alert("JSON copiado!");
    });
    document.getElementById("btn-download-json").addEventListener("click", () => {
      const blob = new Blob([jsonStr], { type: "application/json" });
      const url = URL.createObjectURL(blob);
      const a = document.createElement("a");
      a.href = url;
      a.download = `${state.activeStudy.investigation.id.toLowerCase()}-export.json`;
      a.click();
      URL.revokeObjectURL(url);
    });
  });

  btnGlobalExport.addEventListener("click", async () => {
    const allStudies = await window.tkEngine.getAllStudies();
    const jsonStr = JSON.stringify({
      workspace: "TinyKernel-MultiInvestigation-Workspace",
      schema_version: 1,
      ontology_version: "0.2.0",
      studies: allStudies
    }, null, 2);

    modalTitle.textContent = `Exportação do Workspace Completo (${allStudies.length} Investigações)`;
    modalContent.innerHTML = `
      <p style="color: #94a3b8; font-size: 0.82rem;">Bundle completo com todas as investigações do laboratório.</p>
      <div style="display: flex; gap: 0.75rem; margin-top: 0.5rem;">
        <button class="btn btn-primary btn-sm" id="btn-copy-bundle">Copiar Bundle</button>
        <button class="btn btn-teal btn-sm" id="btn-download-bundle">Baixar tinykernel-workspace.json</button>
      </div>
      <pre class="json-view">${jsonStr}</pre>
    `;
    modalOverlay.classList.add("active");

    document.getElementById("btn-copy-bundle").addEventListener("click", () => {
      navigator.clipboard.writeText(jsonStr);
      alert("Bundle copiado!");
    });
    document.getElementById("btn-download-bundle").addEventListener("click", () => {
      const blob = new Blob([jsonStr], { type: "application/json" });
      const url = URL.createObjectURL(blob);
      const a = document.createElement("a");
      a.href = url;
      a.download = `tinykernel-workspace.json`;
      a.click();
      URL.revokeObjectURL(url);
    });
  });

  btnGlobalImport.addEventListener("click", () => fileImportInput.click());

  fileImportInput.addEventListener("change", (e) => {
    const file = e.target.files[0];
    if (!file) return;
    const reader = new FileReader();
    reader.onload = (event) => {
      try {
        const parsed = JSON.parse(event.target.result);
        if (parsed.studies && Array.isArray(parsed.studies)) {
          parsed.studies.forEach(s => window.tkEngine.saveStudy(s));
          alert(`Workspace com ${parsed.studies.length} investigações importado com sucesso!`);
          renderLabHome();
        } else if (parsed.investigation && parsed.investigation.id) {
          window.tkEngine.saveStudy(parsed);
          alert(`Investigação ${parsed.investigation.id} importada com sucesso!`);
          openStudyWorkbench(parsed.investigation.id);
        } else {
          throw new Error("Formato JSON incompatível com TK-O.");
        }
      } catch (err) {
        alert(`Erro ao importar: ${err.message}`);
      }
    };
    reader.readAsText(file);
  });

  btnModalClose.addEventListener("click", () => modalOverlay.classList.remove("active"));
  modalOverlay.addEventListener("click", (e) => {
    if (e.target === modalOverlay) modalOverlay.classList.remove("active");
  });

  // Initial Route -> Lab Home
  switchView("lab-home");
});
