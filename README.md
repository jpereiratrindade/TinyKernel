# TinyKernel

**Laboratório experimental de estruturas causais minimais**

> **Sempre pronto. Sempre incompleto.**

TinyKernel é método, instrumento e memória experimental. O sistema representa
fenômenos, realizações e intervenções; executa investigações; registra observações e
evidências; limita claims à força da evidência; e mostra a fronteira experimental
conhecida.

A implementação atual materializa a ontologia operacional **TK-O v0.2.1**, os
experimentos de referência `TK-0000` (Sanity) e `TK-0001` (Persistência Adaptativa),
e o benchmark territorial `TK-SAIT-001` (Resiliência de Sistema Agroalimentar Territorial)
sem declarar uma ontologia final ou um Kernel universal.

## Estado

```text
READY
INCOMPLETE BY DESIGN
```

`READY` significa que o estado atual compila, executa, verifica e reproduz os
experimentos publicados. `INCOMPLETE BY DESIGN` significa que novos contextos,
realizações, intervenções, witnesses e revisões ontológicas continuam abertos.

## O que existe

- `libtinykernel`: núcleo C++ independente da apresentação;
- `tinykernel`: CLI para workspace, experimentos, claims, frontier e export;
- `tinykernel-web`: interface única do produto, uma Mesa de Investigação causal interativa que renderiza decisões projetadas pelo núcleo;
- SQLite: memória experimental local (schema 2), com evidência imutável, migração transacional `1 -> 2` e verificação de integridade do envelope completo (`artifact`, `sha256`, `evidence_type`, `run_id`, `witness_id`, `observation_ids`);
- export JSON determinístico com discriminação canônica de tipos de evidência e versão ontológica TK-O v0.2.1;
- ontologia TK-O v0.2.1 versionada com isolamento estrito entre registros estruturais (`STRUCTURAL_RECORD`) e evidências empíricas (`EMPIRICAL_OBSERVATION`);
- máquina de estados de fases monotônica: $\text{FORMULATED} \to \text{MATERIALIZED} \to \text{OBSERVED} \to \text{ADJUDICATED} \to \text{INFERRED}$;
- escada de claims L0–L8 com gates estritos de suficiência (L2) e necessidade relativa (L3 restrito à cadeia $\text{claim.intervention\_scope} \to \text{Run} \to \text{Adjudication(BROKEN\_CAUSAL)} \to \text{Evidence}$);
- adjudicação de observações parciais classificada explicitamente como `PARTIALLY_OBSERVED` / `undetermined` (nunca `PRESERVED`);
- CTest como autoridade única de testes (26 suites automatizadas cobrindo núcleo, fluxo transacional web, renderer no navegador, bridge da API local, migração SQLite, imutabilidade e CLI), executado também pelo GitHub Actions a cada push e pull request.

A cadeia ponta a ponta é:

```text
Phenomenon
→ ConstitutiveProfile + Context
→ Realization
→ Intervention
→ Run
→ Observation
→ Witness
→ Evidence (SHA-256)
→ Adjudication
→ Claim
→ Frontier
```

## Dependências

- Linux;
- compilador com o modo C++26 disponível;
- CMake 3.28 ou posterior;
- Ninja;
- SQLite 3.35 ou posterior, incluindo headers de desenvolvimento;

Em Fedora, os pacotes de desenvolvimento relevantes incluem:

```bash
sudo dnf install cmake ninja-build gcc-c++ sqlite-devel
```

Em Ubuntu/Debian, os nomes usuais são:

```bash
sudo apt install cmake ninja-build g++ libsqlite3-dev
```

Python 3 é utilizado apenas pelo servidor HTTP local da interface web.

## Construir

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build
ctest --test-dir build --output-on-failure
```

O executável de conveniência configura e constrói automaticamente:

```bash
./bin/tinykernel version
```

## Verificar

A partir de um clone limpo com as dependências instaladas:

```bash
./bin/tinykernel verify
```

Esse comando:

1. configura CMake com testes habilitados;
2. constrói o núcleo, CLI e testes;
3. executa todo o CTest, incluindo o renderer web e sua bridge com o núcleo;
4. verifica TK-O, operadores, TK-0000, TK-0001, benchmark TK-SAIT-001, gates de não-implicação causal, causal space, limites de claims, integridade SQLite, digests e export determinístico;
5. retorna status diferente de zero quando qualquer gate falha.

## Executar TK-0001

Crie uma memória experimental local e execute a investigação:

```bash
./bin/tinykernel init ./workspace
./bin/tinykernel --workspace ./workspace run TK-0001
./bin/tinykernel --workspace ./workspace frontier TK-0001
./bin/tinykernel --workspace ./workspace claims TK-0001
./bin/tinykernel --workspace ./workspace export TK-0001 > tk-0001.json
```

TK-0001 investiga **persistência adaptativa**, uma formulação deliberadamente mais
fraca que “aprendizado”:

```text
experiência
→ alteração de estado
→ persistência
→ comportamento posterior alterado
```

O protocolo executa uma baseline e duas trajetórias:

- substituir feedback por uma realização equivalente: `PRESERVED`;
- remover atualização: `BROKEN_CAUSAL`.

Três intervenções permanecem abertas: desabilitar persistência, fundir estado e ação,
e perturbar feedback. A investigação sustenta claims L2 e L3; o claim L5 permanece
aberto. Nenhuma minimalidade global é declarada.

TK-0000, preregistrado antes de TK-0001, verifica somente o aparato:

```bash
./bin/tinykernel --workspace ./workspace experiment TK-0000
```

## CLI

```text
tinykernel version
tinykernel verify
tinykernel init <workspace>
tinykernel [--workspace PATH] list
tinykernel [--workspace PATH] show <investigation> [--json]
tinykernel [--workspace PATH] run <investigation> [--json]
tinykernel [--workspace PATH] frontier <investigation> [--json]
tinykernel [--workspace PATH] claims <investigation> [--json]
tinykernel [--workspace PATH] export <investigation>
```

`--json` está disponível nas consultas analíticas. O export é sempre JSON canônico.

## Interface Web

Para utilizar a interface web com o grafismo e padrão visual do ecossistema **SisTer** (`sisterlocal`, `Sister-Studio`):

```bash
./bin/tinykernel-web
# ou para abrir automaticamente no navegador:
./bin/tinykernel-web --open
# workspace explícito para o núcleo local:
./bin/tinykernel-web --workspace ./workspace --open
```

Esta é a única interface de usuário do TinyKernel. A CLI permanece como ferramenta operacional e de automação. Seus recursos incluem:

- **Nível 1 — Home do Laboratório (`Lab Home`)**: Catálogo geral de investigações, métricas globais e backup/restauração integral do workspace SQLite;
- **Nível 2 — Assistente de Nova Investigação (`Wizard TK-000X`)**: Construtor guiado em 6 passos que bloqueia formulações incompletas e nunca inventa defaults científicos: $(P, C, \Phi) \rightarrow R \rightarrow I \rightarrow W \rightarrow E \rightarrow Q$;
- **Nível 3 — Mesa de Investigação**: abre em “Agora”, com trajetória e próxima ação; Espaço Causal, Evidência, Claims e Definição são revelados sob demanda.

O servidor expõe uma API local em `/api`. `TK-0000`, `TK-0001`, `TK-SAIT-001` e as
investigações criadas no navegador são lidas e escritas pelo núcleo C++/SQLite. Criar,
materializar intervenções, registrar ou revisar observações, adjudicar e inferir
passam pela mesma biblioteca. Evidências antigas permanecem
imutáveis; uma revisão acrescenta um novo envelope e invalida adjudicações e claims
derivados até o recálculo. Estudos com evidência selada não podem ser apagados. A
interface mostra `CORE C++ • READY` quando essa ponte está ativa.

O advisor da investigação, o ranking de intervenções, as prévias contrafactuais e o
depurador de claims vêm de uma única `WorkflowProjection` calculada em C++. O modo
sem servidor exibe apenas uma demonstração somente leitura. O JSON de uma investigação
é entregue diretamente pelo exportador canônico; o workspace completo é baixado e
restaurado como SQLite, com integridade verificada antes da troca.

## Pipeline Epistemológico: Especificação ≠ Observação

O TinyKernel implementa o **Princípio da Não-Implicação Causal**:

$$(P, C, \Phi, R_0, I) \not\Rightarrow E_{\text{empírica}}$$

Uma investigação passa por 5 fases estritas:
1. **DECLARED (`formulated`)**: Especificação formal do fenômeno ($P$), contexto ($C$), perfil ($\Phi$), baseline ($R_0$) e intervenções planejadas ($I$). Produz apenas registros de integridade estrutural (SHA-256 da especificação). Claims permanecem `OPEN`.
2. **MATERIALIZED (`materialized`)**: Construção do aparato ou modelo no espaço operacional.
3. **OBSERVED (`executed`)**: Injeção de medições reais, rastros de campo ou logs empíricos nas realizações.
4. **ADJUDICATED**: Avaliação multidimensional dos *witnesses* constitutivos preregistrados contra os dados observados.
5. **INFERRED**: Sustentação estrita de claims (L2 de suficiência, L3 de necessidade relativa) condicionada a evidências empíricas efetivas.

O benchmark histórico **`TK-SAIT-001`** (Resiliência de Sistema Agroalimentar Territorial) demonstra essa separação: ele nasce em estado `FORMULATED` com baseline `UNTESTED`, 6 intervenções planejadas e claims abertos, impedindo a tautologia de gerar evidências sintéticas a priori. Sua concepção de resiliência como estabilidade/capacidade é preservada como registro histórico; uma hipótese posterior baseada no acoplamento Ecológico–Produtivo–Social deverá receber uma nova identidade (`TK-SAIT-002`), nunca reescrever o benchmark anterior.

## Arquitetura e fundamento

- [TK-ARCH-00 — arquitetura implementada](docs/TK-ARCH-00-arquitetura-experimental-v0.1.0.md)
- [TK-O v0.2.0](ontology/TK-O-0.2.0.yaml)
- [TK-FND-00 v0.1.0](docs/TK-FND-00-fundamentos-minimalidade-causal-v0.1.0.md)
- [TK-FND-00 v0.2.0 — fonte LaTeX](docs/source/TK-FND-00-fundamentos-ontologicos-v0.2.0.tex)
- [TK-NOTE-001](docs/notes/TK-NOTE-001-ontologia-coerencia-perturbacao-reconfiguracao-resiliencia.md)

Os documentos fundadores não são reescritos para acomodar o software. A arquitetura
é uma materialização versionada e revisável dos contratos científicos atuais.

## Licença

GNU General Public License v3.0 (`GPL-3.0-only`).
