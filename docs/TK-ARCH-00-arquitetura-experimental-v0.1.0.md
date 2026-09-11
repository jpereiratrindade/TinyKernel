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
CLI ── libtinykernel ── ontology
                    ├─ causal
                    ├─ experiment + adapter boundary
                    ├─ evidence
                    ├─ knowledge
                    └─ persistence (SQLite + canonical JSON)

Web ── Mesa de Investigação ── API local ── CLI/libtinykernel
                                      ├──── WorkflowProjection
                                      └──── SQLite + JSON canônico
```

O produto possui uma única interface de usuário: a Mesa de Investigação web. A CLI
permanece como superfície operacional do `libtinykernel`. A API local fornece as
investigações canônicas e personalizadas a partir do C++/SQLite. Criação, materialização,
observação, adjudicação e inferência percorrem o fluxo transacional do núcleo; estudos
com evidência selada não podem ser apagados. O advisor, o ranking contrafactual e a
explicação de claims são calculados em `WorkflowProjection` pelo núcleo. Desconectado,
o navegador apresenta referências embarcadas em modo demonstração somente leitura.

Exportações individuais são bytes do `deterministic_export()`; exportação e restauração
integral de workspace usam o banco SQLite canônico, validado antes da substituição.

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
os contratos da interface web com o núcleo) e roda os invariantes internos. Um resultado `READY` significa que o
estado material é reproduzível. As intervenções planejadas e a ausência de evidência
L4–L8 mantêm o sistema `INCOMPLETE BY DESIGN`.
