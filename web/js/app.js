/**
 * TinyKernel Web Application Controller (SisTer Style)
 */

document.addEventListener("DOMContentLoaded", async () => {
  const state = {
    activeStudy: null,
    selectedEntity: null,
    activeRunId: null,
    graph: null,
    experimentKey: "TK-0001"
  };

  // DOM Elements
  const expSelector = document.getElementById("exp-selector");
  const btnRun = document.getElementById("btn-run-investigation");
  const btnExport = document.getElementById("btn-export-json");
  const btnImport = document.getElementById("btn-import-json");
  const btnModalClose = document.getElementById("btn-modal-close");
  const modalOverlay = document.getElementById("modal-overlay");
  const modalTitle = document.getElementById("modal-title");
  const modalContent = document.getElementById("modal-content");
  const fileInput = document.getElementById("file-import-input");

  // Init Graph
  state.graph = new TkCausalGraph("graph-container", (id, raw) => {
    handleEntitySelection(id, raw);
  });

  // Load Initial Experiment
  async function loadExperiment(key) {
    state.experimentKey = key;
    if (key === "TK-0001") {
      state.activeStudy = await window.tkEngine.buildTk0001();
    } else if (key === "TK-0000") {
      state.activeStudy = await window.tkEngine.buildTk0000();
    }
    state.selectedEntity = null;
    state.activeRunId = state.activeStudy.runs.length > 0 ? state.activeStudy.runs[0].id : null;
    renderAll();
  }

  // Handle entity selection across any component
  function handleEntitySelection(id, entity) {
    state.selectedEntity = { id, data: entity };
    renderInspector(id, entity);
  }

  // Render everything
  function renderAll() {
    if (!state.activeStudy) return;

    renderStats();
    renderPhenomenon();
    renderGraph();
    renderFrontierDock();
    renderRuns();
    renderClaims();
    renderFrontierAnalysis();
    renderInspector(state.selectedEntity ? state.selectedEntity.id : null, state.selectedEntity ? state.selectedEntity.data : null);
  }

  // 1. KPI Stats
  function renderStats() {
    const s = state.activeStudy;
    document.getElementById("stat-realizations").textContent = s.realizations.length;
    document.getElementById("stat-interventions").textContent = s.interventions.length;
    document.getElementById("stat-witnesses").textContent = s.witnesses.length;
    document.getElementById("stat-evidence").textContent = s.evidence.length;

    const supportedClaims = s.claims.filter(c => c.status === "supported").length;
    document.getElementById("stat-claims").textContent = `${supportedClaims}/${s.claims.length}`;
  }

  // 2. Phenomenon, Context & Profile
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

  // 3. Graph
  function renderGraph() {
    state.graph.setData(state.activeStudy.realizations, state.activeStudy.interventions);
  }

  // 4. Frontier Dock
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

  // 5. Runs & Evidence
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

  // 6. Claims Ladder
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

  // 7. Frontier Analysis
  function renderFrontierAnalysis() {
    const container = document.getElementById("frontier-analysis-container");
    const s = state.activeStudy;

    const openClaims = s.claims.filter(c => c.status === "open");
    const plannedInterventions = s.interventions.filter(i => i.status !== "performed");

    container.innerHTML = `
      <div class="callout-box warning">
        <strong>Fronteira Epistêmica Aberta</strong>
        <p>As candidatas são minimais apenas no espaço conhecido e sob o perfil &Gamma;.</p>
        <p style="margin-top: 0.4rem; color: #f3bf4f; font-weight: 600;">
          ${plannedInterventions.length} intervenções planejadas impedem promoção a L4–L8.
        </p>
      </div>
      <div class="callout-box">
        <strong>Incompletude por Design</strong>
        <p>A ontologia TK-O v0.2.0 veta saltos indutivos universais sem intervenções empíricas verificadas em testemunhas imutáveis.</p>
      </div>
    `;
  }

  // 8. Inspector Details
  function renderInspector(id, entity) {
    const container = document.getElementById("inspector-details-container");
    const s = state.activeStudy;

    if (!id || !entity) {
      // Default: inspect first run evidence
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

    // Custom entity view
    let detailsHtml = "";
    if (entity.level) {
      // Claim
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
      // Intervention
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
      // Realization
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
      // Generic JSON view
      detailsHtml = `
        <div class="inspector-card">
          <h4>Entidade: ${id}</h4>
          <pre class="json-view">${JSON.stringify(entity, null, 2)}</pre>
        </div>
      `;
    }

    container.innerHTML = detailsHtml;
  }

  // Event Listeners
  expSelector.addEventListener("change", (e) => {
    loadExperiment(e.target.value);
  });

  btnRun.addEventListener("click", async () => {
    btnRun.disabled = true;
    btnRun.textContent = "Executando...";
    await new Promise(r => setTimeout(r, 250)); // Visual feel
    await loadExperiment(state.experimentKey);
    btnRun.disabled = false;
    btnRun.textContent = "Executar Investigação";
  });

  btnExport.addEventListener("click", () => {
    const jsonStr = JSON.stringify(state.activeStudy, null, 2);
    modalTitle.textContent = `Exportação Determinística — ${state.activeStudy.investigation.id}`;
    modalContent.innerHTML = `
      <p style="color: #94a3b8; font-size: 0.82rem;">Este JSON reflete o estado determinístico exportado conforme as regras do TK-O v0.2.0.</p>
      <div style="display: flex; gap: 0.75rem; margin-top: 0.5rem;">
        <button class="btn btn-primary btn-sm" id="btn-copy-json">Copiar JSON</button>
        <button class="btn btn-teal btn-sm" id="btn-download-json">Baixar Arquivo .json</button>
      </div>
      <pre class="json-view" id="export-json-view">${jsonStr}</pre>
    `;
    modalOverlay.classList.add("active");

    document.getElementById("btn-copy-json").addEventListener("click", () => {
      navigator.clipboard.writeText(jsonStr);
      alert("JSON copiado para a área de transferência!");
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

  btnImport.addEventListener("click", () => {
    fileInput.click();
  });

  fileInput.addEventListener("change", (e) => {
    const file = e.target.files[0];
    if (!file) return;
    const reader = new FileReader();
    reader.onload = (event) => {
      try {
        const importedStudy = JSON.parse(event.target.result);
        if (!importedStudy.investigation || !importedStudy.investigation.id) {
          throw new Error("Arquivo JSON inválido para o formato TK-O.");
        }
        state.activeStudy = importedStudy;
        state.experimentKey = importedStudy.investigation.id;
        renderAll();
        alert(`Workspace importado com sucesso: ${importedStudy.investigation.id}`);
      } catch (err) {
        alert(`Erro ao importar arquivo: ${err.message}`);
      }
    };
    reader.readAsText(file);
  });

  btnModalClose.addEventListener("click", () => {
    modalOverlay.classList.remove("active");
  });

  modalOverlay.addEventListener("click", (e) => {
    if (e.target === modalOverlay) {
      modalOverlay.classList.remove("active");
    }
  });

  // Initial Boot
  await loadExperiment("TK-0001");
});
