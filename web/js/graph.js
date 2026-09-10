/**
 * Causal Space Interactive Graph Renderer — GP = (R, I)
 * SisTer Aesthetic with SVG bezier curves, interactive node dragging, and outcome color-coding.
 */

class TkCausalGraph {
  constructor(containerId, onSelectEntity) {
    this.container = document.getElementById(containerId);
    this.onSelectEntity = onSelectEntity;
    this.svg = null;
    this.nodes = [];
    this.edges = [];
    this.selectedId = null;
    this.draggedNode = null;
    this.dragOffset = { x: 0, y: 0 };
    this.init();
  }

  init() {
    if (!this.container) return;
    this.container.innerHTML = "";

    this.svg = document.createElementNS("http://www.w3.org/2000/svg", "svg");
    this.svg.setAttribute("class", "graph-svg");
    this.svg.setAttribute("width", "100%");
    this.svg.setAttribute("height", "100%");
    this.svg.style.overflow = "visible";

    // Defs for arrowheads and gradients
    const defs = document.createElementNS("http://www.w3.org/2000/svg", "defs");
    defs.innerHTML = `
      <marker id="arrow" viewBox="0 0 10 10" refX="8" refY="5" markerWidth="6" markerHeight="6" orient="auto-start-reverse">
        <path d="M 0 1.5 L 10 5 L 0 8.5 z" fill="#38bdf8" />
      </marker>
      <marker id="arrow-ruptured" viewBox="0 0 10 10" refX="8" refY="5" markerWidth="6" markerHeight="6" orient="auto-start-reverse">
        <path d="M 0 1.5 L 10 5 L 0 8.5 z" fill="#f87171" />
      </marker>
      <filter id="glow" x="-20%" y="-20%" width="140%" height="140%">
        <feGaussianBlur stdDeviation="3" result="blur" />
        <feComposite in="SourceGraphic" in2="blur" operator="over" />
      </filter>
    `;
    this.svg.appendChild(defs);

    this.edgeGroup = document.createElementNS("http://www.w3.org/2000/svg", "g");
    this.edgeGroup.setAttribute("class", "edges-layer");
    this.svg.appendChild(this.edgeGroup);

    this.nodeGroup = document.createElementNS("http://www.w3.org/2000/svg", "g");
    this.nodeGroup.setAttribute("class", "nodes-layer");
    this.svg.appendChild(this.nodeGroup);

    this.container.appendChild(this.svg);

    // Global SVG mouse listeners for smooth node dragging
    this.svg.addEventListener("mousemove", (e) => this.onMouseMove(e));
    this.svg.addEventListener("mouseup", () => this.onMouseUp());
    this.svg.addEventListener("mouseleave", () => this.onMouseUp());
  }

  setData(realizations, interventions) {
    const width = this.container.clientWidth || 650;
    const height = this.container.clientHeight || 360;

    // Normalizing positions if not preset
    this.nodes = realizations.map((r, i) => {
      let x = r.x !== undefined ? r.x : (i === 0 ? 50 : 380);
      let y = r.y !== undefined ? r.y : (i === 0 ? 110 : (i === 1 ? 40 : 190));
      // Clamp bounds
      x = Math.max(20, Math.min(width - 220, x));
      y = Math.max(20, Math.min(height - 110, y));

      return {
        id: r.id,
        label: r.label,
        components: r.components || [],
        complexity: r.complexity || 0,
        outcome: r.outcome || "pending",
        x: x,
        y: y,
        width: 190,
        height: 74,
        raw: r
      };
    });

    this.edges = interventions.map((itv) => ({
      id: itv.id,
      kind: itv.kind,
      source: itv.source,
      target: itv.target,
      prediction: itv.prediction,
      status: itv.status,
      target_component: itv.target_component,
      replacement_component: itv.replacement_component,
      raw: itv
    }));

    this.render();
  }

  render() {
    if (!this.svg) return;
    this.edgeGroup.innerHTML = "";
    this.nodeGroup.innerHTML = "";

    // 1. Render Edges (Interventions)
    this.edges.forEach((edge) => {
      if (edge.status !== "performed") return;

      const srcNode = this.nodes.find(n => n.id === edge.source);
      const tgtNode = this.nodes.find(n => n.id === edge.target);
      if (!srcNode || !tgtNode) return;

      const sx = srcNode.x + srcNode.width;
      const sy = srcNode.y + srcNode.height / 2;
      const tx = tgtNode.x;
      const ty = tgtNode.y + tgtNode.height / 2;

      const dx = tx - sx;
      const cp1x = sx + dx * 0.45;
      const cp1y = sy;
      const cp2x = sx + dx * 0.55;
      const cp2y = ty;

      const path = document.createElementNS("http://www.w3.org/2000/svg", "path");
      const d = `M ${sx} ${sy} C ${cp1x} ${cp1y}, ${cp2x} ${cp2y}, ${tx} ${ty}`;
      path.setAttribute("d", d);
      path.setAttribute("fill", "none");
      path.setAttribute("stroke", edge.prediction === "BROKEN_CAUSAL" ? "#f87171" : "#38bdf8");
      path.setAttribute("stroke-width", "2.5");
      path.setAttribute("stroke-dasharray", edge.prediction === "BROKEN_CAUSAL" ? "5,3" : "none");
      path.setAttribute("marker-end", edge.prediction === "BROKEN_CAUSAL" ? "url(#arrow-ruptured)" : "url(#arrow)");
      path.style.transition = "stroke 0.2s";
      this.edgeGroup.appendChild(path);

      // Midpoint Intervention Chip / Badge
      const mx = (sx + tx) / 2;
      const my = (sy + ty) / 2;

      const badgeGroup = document.createElementNS("http://www.w3.org/2000/svg", "g");
      badgeGroup.setAttribute("class", "graph-edge-badge");
      badgeGroup.setAttribute("transform", `translate(${mx}, ${my})`);
      badgeGroup.style.cursor = "pointer";

      const bgRect = document.createElementNS("http://www.w3.org/2000/svg", "rect");
      const labelText = `${edge.kind}: ${edge.target_component || ""}`;
      const badgeWidth = Math.max(90, labelText.length * 7.5 + 16);
      bgRect.setAttribute("x", -badgeWidth / 2);
      bgRect.setAttribute("y", -13);
      bgRect.setAttribute("width", badgeWidth);
      bgRect.setAttribute("height", 26);
      bgRect.setAttribute("rx", "13");
      bgRect.setAttribute("fill", "#0f172a");
      bgRect.setAttribute("stroke", this.selectedId === edge.id ? "#38bdf8" : "rgba(148, 163, 184, 0.4)");
      bgRect.setAttribute("stroke-width", this.selectedId === edge.id ? "2" : "1");
      badgeGroup.appendChild(bgRect);

      const text = document.createElementNS("http://www.w3.org/2000/svg", "text");
      text.setAttribute("x", 0);
      text.setAttribute("y", 4);
      text.setAttribute("text-anchor", "middle");
      text.setAttribute("fill", this.selectedId === edge.id ? "#38bdf8" : "#cbd5e1");
      text.setAttribute("font-size", "11px");
      text.setAttribute("font-family", "Inter, sans-serif");
      text.setAttribute("font-weight", "600");
      text.textContent = labelText;
      badgeGroup.appendChild(text);

      badgeGroup.addEventListener("click", (e) => {
        e.stopPropagation();
        this.select(edge.id, edge.raw);
      });

      this.edgeGroup.appendChild(badgeGroup);
    });

    // 2. Render Nodes (Realizations)
    this.nodes.forEach((node) => {
      const g = document.createElementNS("http://www.w3.org/2000/svg", "g");
      g.setAttribute("class", "graph-node");
      g.setAttribute("transform", `translate(${node.x}, ${node.y})`);
      g.style.cursor = "move";

      const isSelected = this.selectedId === node.id;
      const isPreserved = node.outcome === "preserving";
      const isRuptured = node.outcome === "ruptured";

      // Outer Card Box
      const rect = document.createElementNS("http://www.w3.org/2000/svg", "rect");
      rect.setAttribute("width", node.width);
      rect.setAttribute("height", node.height);
      rect.setAttribute("rx", "10");
      
      const bgColor = isPreserved ? "rgba(29, 80, 65, 0.88)" : (isRuptured ? "rgba(101, 57, 56, 0.88)" : "rgba(30, 41, 59, 0.85)");
      const borderColor = isSelected ? "#38bdf8" : (isPreserved ? "#34d399" : (isRuptured ? "#f87171" : "rgba(148, 163, 184, 0.3)"));

      rect.setAttribute("fill", bgColor);
      rect.setAttribute("stroke", borderColor);
      rect.setAttribute("stroke-width", isSelected ? "2.5" : "1.2");
      if (isSelected) rect.setAttribute("filter", "url(#glow)");
      g.appendChild(rect);

      // Node Label
      const title = document.createElementNS("http://www.w3.org/2000/svg", "text");
      title.setAttribute("x", "14");
      title.setAttribute("y", "26");
      title.setAttribute("fill", "#ffffff");
      title.setAttribute("font-size", "12px");
      title.setAttribute("font-weight", "700");
      title.setAttribute("font-family", "Inter, sans-serif");
      title.textContent = node.label;
      g.appendChild(title);

      // ID Subtitle
      const sub = document.createElementNS("http://www.w3.org/2000/svg", "text");
      sub.setAttribute("x", "14");
      sub.setAttribute("y", "44");
      sub.setAttribute("fill", "#94a3b8");
      sub.setAttribute("font-size", "10px");
      sub.setAttribute("font-family", "JetBrains Mono, monospace");
      sub.textContent = node.id.split(":").slice(2).join(":");
      g.appendChild(sub);

      // Outcome Badge Pill
      const badgeY = 52;
      const statusText = isPreserved ? "PRESERVED" : (isRuptured ? "RUPTURED" : "PENDING");
      const statusColor = isPreserved ? "#34d399" : (isRuptured ? "#f87171" : "#94a3b8");

      const statusTag = document.createElementNS("http://www.w3.org/2000/svg", "text");
      statusTag.setAttribute("x", "14");
      statusTag.setAttribute("y", badgeY + 12);
      statusTag.setAttribute("fill", statusColor);
      statusTag.setAttribute("font-size", "9.5px");
      statusTag.setAttribute("font-weight", "800");
      statusTag.setAttribute("letter-spacing", "0.06em");
      statusTag.textContent = statusText;
      g.appendChild(statusTag);

      // Complexity indicator on right
      const comp = document.createElementNS("http://www.w3.org/2000/svg", "text");
      comp.setAttribute("x", node.width - 14);
      comp.setAttribute("y", badgeY + 12);
      comp.setAttribute("text-anchor", "end");
      comp.setAttribute("fill", "#38bdf8");
      comp.setAttribute("font-size", "10px");
      comp.setAttribute("font-family", "JetBrains Mono, monospace");
      comp.textContent = `|Σ|=${node.components.length}`;
      g.appendChild(comp);

      // Drag and selection event handlers
      g.addEventListener("mousedown", (e) => {
        e.stopPropagation();
        this.draggedNode = node;
        const rectBound = this.svg.getBoundingClientRect();
        this.dragOffset.x = (e.clientX - rectBound.left) - node.x;
        this.dragOffset.y = (e.clientY - rectBound.top) - node.y;
        this.select(node.id, node.raw);
      });

      this.nodeGroup.appendChild(g);
    });
  }

  onMouseMove(e) {
    if (!this.draggedNode) return;
    const rect = this.svg.getBoundingClientRect();
    const mouseX = e.clientX - rect.left;
    const mouseY = e.clientY - rect.top;

    this.draggedNode.x = Math.max(10, Math.min(rect.width - this.draggedNode.width - 10, mouseX - this.dragOffset.x));
    this.draggedNode.y = Math.max(10, Math.min(rect.height - this.draggedNode.height - 10, mouseY - this.dragOffset.y));

    this.render();
  }

  onMouseUp() {
    this.draggedNode = null;
  }

  select(id, rawEntity) {
    this.selectedId = id;
    this.render();
    if (this.onSelectEntity) {
      this.onSelectEntity(id, rawEntity);
    }
  }
}

if (typeof window !== "undefined") {
  window.TkCausalGraph = TkCausalGraph;
}
if (typeof module !== "undefined" && module.exports) {
  module.exports = { TkCausalGraph };
}
