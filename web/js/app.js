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
  const btnWorkbenchNewIntervention = document.getElementById("btn-workbench-new-intervention");
  const btnWorkbenchRun = document.getElementById("btn-workbench-run");
  const btnWorkbenchExport = document.getElementById("btn-workbench-export");

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
    const studies = await window.tkEngine.getAllStudies();

    // Global Stats
    let totalEvidence = 0;
    let totalClaims = 0;
    studies.forEach(s => {
      totalEvidence += (s.evidence || []).length;
      totalClaims += (s.claims || []).filter(c => c.status === "supported").length;
    });

    document.getElementById("global-stat-investigations").textContent = studies.length;
    document.getElementById("global-stat-evidence").textContent = totalEvidence;
    document.getElementById("global-stat-claims").textContent = totalClaims;

    const grid = document.getElementById("investigations-grid-container");
    grid.innerHTML = "";

    studies.forEach(study => {
      const inv = study.investigation;
      const isCanonical = inv.id === "TK-0001";
      const isSanity = inv.id === "TK-0000";
      
      const badgeCategory = isCanonical ? "canonical" : (isSanity ? "sanity" : "user");
      const badgeText = isCanonical ? "Canônico • Referência" : (isSanity ? "Sanity • Bootstrap" : "Investigação Inédita");

      const supportedClaims = (study.claims || []).filter(c => c.status === "supported").length;

      const card = document.createElement("div");
      card.className = "investigation-card";
      card.innerHTML = `
        <div class="card-top">
          <span class="card-id">${inv.id}</span>
          <span class="card-category-badge ${badgeCategory}">${badgeText}</span>
        </div>
        <div class="card-title">${inv.title || study.phenomenon.name}</div>
        <div class="card-desc">${study.phenomenon.description || "Sem descrição"}</div>
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
            <span class="label">Evidências</span>
            <span class="value">${(study.evidence || []).length}</span>
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
  }

  async function openStudyWorkbench(studyId) {
    state.activeStudyId = studyId;
    state.activeStudy = await window.tkEngine.getStudy(studyId);
    state.selectedEntity = null;
    state.activeRunId = state.activeStudy && state.activeStudy.runs.length ? state.activeStudy.runs[0].id : null;
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
    document.getElementById("stat-witnesses").textContent = s.witnesses.length;
    document.getElementById("stat-evidence").textContent = s.evidence.length;

    const supportedClaims = s.claims.filter(c => c.status === "supported").length;
    document.getElementById("stat-claims").textContent = `${supportedClaims}/${s.claims.length}`;

    const executedItvs = s.interventions.filter(i => i.status === "performed").length;
    const plannedItvs = s.interventions.length - executedItvs;
    document.getElementById("stat-interventions-caption").textContent = `${executedItvs} executadas • ${plannedItvs} planejadas`;
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
    const dock = document.getElementById("frontier-chips-container");
    dock.innerHTML = "";
    const s = state.activeStudy;

    s.interventions.forEach(itv => {
      const isPerformed = itv.status === "performed";
      const chip = document.createElement("div");
      chip.className = `frontier-chip ${isPerformed ? "performed" : "open-tag"}`;
      chip.innerHTML = `
        <span>${isPerformed ? "✓" : "○"}</span>
        <strong>${itv.kind}</strong>
        <span>${itv.target_component ? `· ${itv.target_component}` : ""}</span>
      `;
      chip.title = `ID: ${itv.id}\nPredição: ${itv.prediction}\nStatus: ${itv.status}`;
      chip.addEventListener("click", () => {
        handleEntitySelection(itv.id, itv);
      });
      dock.appendChild(chip);
    });
  }

  function renderRuns() {
    const list = document.getElementById("runs-list-container");
    list.innerHTML = "";
    const s = state.activeStudy;

    s.runs.forEach(run => {
      const adj = s.adjudications.find(a => a.run_id === run.id);
      const isPreserved = adj && adj.outcome === "preserving";
      const outcomeText = adj ? adj.classification : "PENDING";
      const evidenceForRun = s.evidence.filter(e => e.run_id === run.id);

      const card = document.createElement("div");
      card.className = `run-card ${state.activeRunId === run.id ? "active" : ""}`;
      card.innerHTML = `
        <div class="run-card-header">
          <span class="run-id">${run.id.split(":").slice(2).join(":")}</span>
          <span class="run-status-pill ${isPreserved ? "preserved" : "broken"}">${outcomeText}</span>
        </div>
        <div class="run-card-meta">
          <span>${evidenceForRun.length} evidências SHA-256</span>
          <span>•</span>
          <span>${adj ? adj.outcome : "pending"}</span>
        </div>
      `;

      card.addEventListener("click", () => {
        state.activeRunId = run.id;
        renderRuns();
        handleEntitySelection(run.id, { run, adjudication: adj, evidence: evidenceForRun });
      });

      list.appendChild(card);
    });
  }

  function renderClaims() {
    const tbody = document.getElementById("claims-table-body");
    tbody.innerHTML = "";
    const s = state.activeStudy;

    s.claims.forEach(claim => {
      const tr = document.createElement("tr");
      tr.style.cursor = "pointer";
      const isSupported = claim.status === "supported";

      tr.innerHTML = `
        <td><span class="claim-level-pill">${claim.level}</span></td>
        <td><strong>${claim.subject}</strong></td>
        <td>${claim.assertion}</td>
        <td>
          <span class="claim-status ${isSupported ? "supported" : "open"}">
            ${isSupported ? "✓ SUPPORTED" : "○ OPEN"}
          </span>
        </td>
      `;

      tr.addEventListener("click", () => {
        handleEntitySelection(claim.id, claim);
      });

      tbody.appendChild(tr);
    });
  }

  function renderFrontierAnalysis() {
    const container = document.getElementById("frontier-analysis-container");
    const s = state.activeStudy;
    const plannedInterventions = s.interventions.filter(i => i.status !== "performed");

    container.innerHTML = `
      <div class="callout-box warning">
        <strong>Fronteira Epistêmica Aberta</strong>
        <p>As candidatas são minimais apenas no espaço conhecido e sob a ordem &Gamma; declarada.</p>
        <p style="margin-top: 0.4rem; color: #f3bf4f; font-weight: 600;">
          ${plannedInterventions.length > 0 ? `${plannedInterventions.length} intervenções planejadas impedem promoção a L4–L8.` : "Espaço aberto para novas intervenções empíricas."}
        </p>
      </div>
      <div class="callout-box">
        <strong>Incompletude por Design</strong>
        <p>A ontologia TK-O v0.2.0 veta saltos indutivos universais sem intervenções empíricas verificadas em testemunhas imutáveis.</p>
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
          <div><strong>Classificação:</strong> ${adj ? adj.classification : "—"}</div>
          <div><strong>Regra Adjudicada:</strong> <code>${adj ? adj.rule : "—"}</code></div>
          <div><strong>Justificativa:</strong> ${adj ? adj.rationale : "—"}</div>
          <div style="margin-top: 0.5rem;"><strong>Evidências Verificadas (SHA-256):</strong></div>
          ${evs.map(ev => `
            <div style="margin-top: 0.35rem;">
              <span style="color: #94a3b8; font-size: 0.72rem;">${ev.id.split(":").slice(3).join(":")}:</span>
              <div class="hash-preview">${ev.sha256}</div>
            </div>
          `).join("")}
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
          <div><strong>Status:</strong> ${entity.status.toUpperCase()}</div>
          <div><strong>Limitações:</strong> ${entity.limitations || "Nenhuma declarada"}</div>
          <div style="margin-top: 0.4rem;"><strong>Testemunhas:</strong> ${(entity.witness_scope || []).join(", ") || "Nenhuma"}</div>
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
          <div><strong>Status:</strong> ${entity.status}</div>
        </div>
      `;
    } else if (entity.components) {
      detailsHtml = `
        <div class="inspector-card">
          <h4>Realização: ${entity.id}</h4>
          <div><strong>Rótulo:</strong> ${entity.label}</div>
          <div><strong>Complexidade (|&Sigma;|):</strong> ${entity.components.length}</div>
          <div><strong>Resultado Causal:</strong> <span style="color: ${entity.outcome === "preserving" ? "#34d399" : "#f87171"}; font-weight: 700;">${(entity.outcome || "").toUpperCase()}</span></div>
          <div style="margin-top: 0.4rem;"><strong>Componentes Ativos:</strong></div>
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
  // GLOBAL LISTENERS
  // ========================================================
  btnBrandHome.addEventListener("click", () => switchView("lab-home"));
  btnWorkbenchBackToLab.addEventListener("click", () => switchView("lab-home"));
  btnNavNewInvestigation.addEventListener("click", () => switchView("wizard"));
  btnHeroNewInvestigation.addEventListener("click", () => switchView("wizard"));
  btnHeroOpenCanonical.addEventListener("click", () => openStudyWorkbench("TK-0001"));

  btnWorkbenchRun.addEventListener("click", async () => {
    btnWorkbenchRun.disabled = true;
    btnWorkbenchRun.textContent = "Reexecutando...";
    await new Promise(r => setTimeout(r, 200));
    renderWorkbench();
    btnWorkbenchRun.disabled = false;
    btnWorkbenchRun.textContent = "▶ Reexecutar Estudo";
  });

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
