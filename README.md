# TinyKernel

**Laboratório experimental de estruturas causais minimais**

> **Sempre pronto. Sempre incompleto.**

TinyKernel é método, instrumento e memória experimental. O sistema representa
fenômenos, realizações e intervenções; executa investigações; registra observações e
evidências; limita claims à força da evidência; e mostra a fronteira experimental
conhecida.

A implementação atual materializa a ontologia operacional **TK-O v0.2.0**, os
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
- `tinykernel-gui`: interface Qt Quick do mesmo núcleo;
- `tinykernel-web`: interface web interativa no grafismo do ecossistema SisTer;
- SQLite: memória experimental local, com evidence imutável;
- export JSON determinístico;
- ontologia TK-O versionada com discriminação entre registros estruturais e evidências empíricas;
- escada de claims L0–L8 com gates estritos de suficiência (L2) e necessidade relativa (L3);
- CTest como autoridade única de testes (15 suites automatizadas).

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
- Qt 6.5 ou posterior com Core, Gui, Qml, Quick e Quick Controls 2.

Em Fedora, os pacotes de desenvolvimento relevantes incluem:

```bash
sudo dnf install cmake ninja-build gcc-c++ sqlite-devel \
  qt6-qtbase-devel qt6-qtdeclarative-devel
```

Em Ubuntu/Debian, os nomes usuais são:

```bash
sudo apt install cmake ninja-build g++ libsqlite3-dev \
  qt6-base-dev qt6-declarative-dev
```

O configure falha explicitamente quando a GUI está habilitada e os módulos Qt Quick
de desenvolvimento não estão disponíveis. Para trabalho isolado no núcleo, use
`-DTINYKERNEL_BUILD_GUI=OFF`; esse modo não satisfaz sozinho o READY contract
completo.

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

1. configura CMake com GUI e testes habilitados;
2. constrói o núcleo, CLI, GUI e testes;
3. executa todo o CTest, incluindo smoke de startup da GUI e workflow;
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
tinykernel gui [workspace]
```

`--json` está disponível nas consultas analíticas. O export é sempre JSON canônico.

## GUI (Desktop Qt Quick)

Abra a interface gráfica desktop do laboratório:

```bash
./bin/tinykernel gui ./workspace
```

A interface desktop incorpora a mesma arquitetura em 3 níveis:

1. **Home do Laboratório**: Catálogo de investigações disponíveis no workspace (`TK-0000`, `TK-0001` e investigações do usuário), estatísticas consolidadas e métricas de evidência SHA-256;
2. **Wizard de Formulação**: Assistente para criar novas investigações genéricas sem necessidade de programar código C++;
3. **Workbench Analítico**: Espaço Causal $G_P = (R, I)$ interativo com nós arrastáveis, pílulas de intervenção, adição dinâmica de novas intervenções sob demanda, runs com evidências imutáveis e escada de claims (L0–L8).

## Interface Web (SisTer)

Para utilizar a interface web com o grafismo e padrão visual do ecossistema **SisTer** (`sisterlocal`, `Sister-Studio`):

```bash
./bin/tinykernel-web
# ou para abrir automaticamente no navegador:
./bin/tinykernel-web --open
```

Arquitetura e Recursos da Interface Web:
- **Nível 1 — Home do Laboratório (`Lab Home`)**: Catálogo geral de investigações (`TK-0000`, `TK-0001` de calibração e investigações do usuário), métricas globais e exportação/importação de workspaces;
- **Nível 2 — Assistente de Nova Investigação (`Wizard TK-000X`)**: Construtor guiado em 6 passos para formular novas perguntas científicas sem codificação: $(P, C, \Phi) \rightarrow R \rightarrow I \rightarrow W \rightarrow E \rightarrow Q$;
- **Nível 3 — Workbench Analítico**: Detalhe científico com Grafo Causal $G_P = (R, I)$ vetorial interativo, adição dinâmica de intervenções (`remove`, `replace`, `disable`, `merge`, `perturb`), runs determinísticos, verificador criptográfico SHA-256 e escada de claims (L0–L8).

## Pipeline Epistemológico: Especificação ≠ Observação

O TinyKernel implementa o **Princípio da Não-Implicação Causal**:

$$(P, C, \Phi, R_0, I) \not\Rightarrow E_{\text{empírica}}$$

Uma investigação passa por 5 fases estritas:
1. **DECLARED (`formulated`)**: Especificação formal do fenômeno ($P$), contexto ($C$), perfil ($\Phi$), baseline ($R_0$) e intervenções planejadas ($I$). Produz apenas registros de integridade estrutural (SHA-256 da especificação). Claims permanecem `OPEN`.
2. **MATERIALIZED (`materialized`)**: Construção do aparato ou modelo no espaço operacional.
3. **OBSERVED (`executed`)**: Injeção de medições reais, rastros de campo ou logs empíricos nas realizações.
4. **ADJUDICATED**: Avaliação multidimensional dos *witnesses* constitutivos preregistrados contra os dados observados.
5. **INFERRED**: Sustentação estrita de claims (L2 de suficiência, L3 de necessidade relativa) condicionada a evidências empíricas efetivas.

O benchmark **`TK-SAIT-001`** (Resiliência de Sistema Agroalimentar Territorial) demonstra essa separação: ele nasce em estado `FORMULATED` com baseline `UNTESTED`, 6 intervenções planejadas e claims abertos, impedindo a tautologia de gerar evidências sintéticas a priori.

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
