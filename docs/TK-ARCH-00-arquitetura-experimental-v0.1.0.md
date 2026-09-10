---
document_id: TK-ARCH-00
version: 0.1.0
status: IMPLEMENTED
ontology: TK-O v0.2.0
system: TinyKernel 0.1.0
---

# TK-ARCH-00 — Arquitetura experimental

Esta arquitetura materializa TK-SYS-00 a partir de TK-FND-00 v0.2.0. Ela é um
instrumento revisável, não uma ontologia final nem evidência de que um Kernel exista.

## Boundaries

```text
CLI ─────┐
         ├── libtinykernel ── ontology
Qt Quick ┘                   ├─ causal
                            ├─ experiment + adapter boundary
                            ├─ evidence
                            ├─ knowledge
                            └─ persistence (SQLite + canonical JSON)
```

`libtinykernel` não depende de Qt. CLI e GUI leem o mesmo agregado `Study` e usam a
mesma análise de causal space, claims e frontier. QML recebe projeções prontas para
apresentação; não adjudica evidência nem classifica realizações.

## Ontologia materializada

Todos os tipos carregam `id`, `schema_version` e `ontology_version`. A definição
legível por máquina está em `ontology/TK-O-0.2.0.yaml` e inclui os tipos exigidos por
TK-SYS-00, a escada L0–L8 e os cinco operadores iniciais.

O agregado de investigação preserva separadamente fenômeno, contexto, perfil
constitutivo, realização, estrutura, intervenção, observação, witness, evidence,
adjudicação, claim, run e proveniência.

## Causal space e redução

O grafo usa realizações como nós e intervenções executadas como arestas dirigidas.
O campo `reduction_rank` implementa a ordem preregistrada `Γ=active_causal_relations`
de TK-0001. Intervenções de mesmo rank podem conectar duas realizações sem estabelecer
redução estrita; por isso alternativas preservadoras de mesmo rank são reportadas
como incomparáveis sob `Γ`.

`InterventionEngine` mantém um registry de operadores. `remove`, `replace`, `disable`,
`merge` e `perturb` são registros iniciais; um novo operador pode ser registrado sem
alterar o algoritmo do grafo ou a interface do engine.

## Adapter boundary

`RealizationAdapter::observe(Realization)` é o contrato mínimo para realizações
externas. O adapter retorna somente as cinco dimensões observadas e um artefato de
trace. O laboratório continua responsável por criar runs, observações, digests,
evidências e adjudicações. TinyLogicLM e TinyLogicVision podem futuramente implementar
esse contrato sem introduzir suas regras na engine.

## Evidência, claims e frontier

Cada dimensão observada produz um registro de evidência com SHA-256. O schema SQLite
possui triggers `BEFORE UPDATE` e `BEFORE DELETE`; a API também compara conteúdo e
digest antes de aceitar um ID existente.

`ClaimAdjudicator` aplica a escada L0–L8. TK-0001 sustenta L2 e L3. L4/L5 são
bloqueados enquanto houver reduções declaradas não executadas; L6–L8 requerem provas
que o experimento não possui.

A frontier é uma projeção derivada, não uma tabela autoritativa. Ela distingue
realizações conhecidas, preservadoras, rompidas, indeterminadas, candidatas minimais
atuais, candidatas incomparáveis, intervenções não exploradas e claims abertos ou
sustentados. Ela sempre conserva o limite de escopo e nunca infere minimalidade global.

## Persistência e export

SQLite armazena identidades, atributos versionados, relações explícitas, runs,
evidence, claims, scopes e proveniência. `PRAGMA foreign_keys` e `integrity_check`
participam do gate operacional.

O export JSON ordena entidades por ID, usa uma ordem fixa de chaves e preserva a
ordem semântica de componentes, observações e scopes. Repetir o export do mesmo estado
produz bytes idênticos.

## READY e incompletude

`./bin/tinykernel verify` configura, constrói, executa todos os testes CTest (incluindo
startup Qt Quick) e roda os invariantes internos. Um resultado `READY` significa que o
estado material é reproduzível. As intervenções planejadas e a ausência de evidência
L4–L8 mantêm o sistema `INCOMPLETE BY DESIGN`.
