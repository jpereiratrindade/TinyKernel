# TinyKernel

**Laboratório experimental de minimalidade causal**

> **Sempre pronto. Sempre incompleto.**

TinyKernel investiga uma pergunta deliberadamente aberta:

> **O que precisa ser causalmente preservado para que um fenômeno continue
> sendo aquilo que é — inclusive quando sua configuração pode mudar?**

O projeto não presume que exista um Kernel único, universal ou previamente
identificável.

Seu objeto fundador é a **minimalidade causal**.

Sua fronteira conceitual atual acrescenta uma hipótese ainda aberta:

> **talvez o mínimo relevante não seja aquilo que permanece imóvel, mas aquilo
> que preserva a possibilidade de reconfiguração coerente.**

Este README é uma **projeção do estado atual da pesquisa**.
A autoridade epistemológica permanece nos documentos versionados.

---

## Linhagem

TinyKernel nasce do encontro entre:

- **Sister-Kernel** — identidade, primitivas, invariantes e evolução;
- **TinyLogicLM** — capacidade aprendida como cadeia causal observável;
- **TinyLogicVision** — observação, representação, inferência, proveniência e
  distinções semânticas que não podem ser colapsadas sem perda.

TinyKernel é um laboratório independente. Resultados futuros não são
automaticamente transferidos a outros projetos.

---

## Fundamento — TK-FND-00

O documento fundador muda a pergunta de:

> quais são os componentes mínimos?

para:

> **qual é o menor circuito causal que ainda produz o fenômeno?**

Ele estabelece a **minimalidade causal** como objeto de pesquisa e introduz,
como elementos operacionais candidatos:

```text
fenômeno
contexto
realização
witness
intervenção
suficiência
necessidade relativa
irredutibilidade
causalidade
significado
observabilidade
realizações alternativas
```

Sua inversão experimental inicial é:

```text
construir <-> remover
```

O `TK-FND-00` não demonstra que Kernel exista, seja único ou possa ser
descoberto por simples ablação.

---

## Fronteira aberta — TK-NOTE-001

A nota sucessora introduz uma hipótese dinâmica:

> **Resiliência é manter-se coerente enquanto se reconfigura diante da
> perturbação.**

O termo **enquanto** é central.

A reconfiguração passa a ser considerada parte do mecanismo candidato:

```text
ONTOLOGIA CANDIDATA
    ↓
COERÊNCIA CANDIDATA
    ↓
PERTURBAÇÃO
    ↓
DESLOCAMENTO
    ↓
RECONFIGURAÇÃO
    ↓
TRAJETÓRIA
    ↓
COERÊNCIA ATRAVÉS DA MUDANÇA?
    ↓
WITNESS
    ↓
EVIDÊNCIA
    ↓
REVISÃO
```

A `TK-NOTE-001` permanece:

```text
CONCEPTUAL BRIDGE / OPEN / NOT ADOPTED
```

Ela não possui autoridade de implementação.

---

## Questões emergentes

A primeira tensão experimental é:

```text
necessidade em configuração fixa
!=
necessidade em sistema reconfigurável
```

Um elemento pode parecer necessário quando é removido e o sistema é observado
imediatamente, mas revelar-se substituível quando existe oportunidade de
reconfiguração.

Exemplo conceitual:

```text
A -> B -> C
```

Após remover `B`:

```text
A -> ? -> C
```

pode haver falha imediata.

Mas, sob reconfiguração:

```text
A -> X -> C
```

pode emergir uma realização alternativa coerente.

Isso não demonstra que `B` seja dispensável. Cria uma pergunta:

> **A necessidade observada em uma configuração permanece necessária quando o
> sistema pode se reconfigurar?**

A segunda tensão é:

```text
coerência de estado
!=
necessariamente
coerência de trajetória
```

Uma trajetória candidata é:

```text
S_t
-> perturbação
-> deslocamento
-> reconfiguração
-> S_t+1
```

Não se exige:

```text
S_t+1 = S_t
```

A pergunta pode ser:

> **a trajetória de transformação preserva continuidade causal suficiente para
> que o fenômeno permaneça coerente enquanto se reconfigura?**

Essas questões permanecem abertas.

---

## Hipóteses em tensão

TinyKernel preserva atualmente três possibilidades candidatas:

1. **Minimalidade configuracional** — uma realização pode ser localmente
   irredutível sob um contexto, witnesses e protocolo explicitados.

2. **Minimalidade reconfiguracional** — o mínimo relevante pode depender da
   possibilidade de reorganização após uma perturbação.

3. **Minimalidade de trajetória** — a unidade relevante pode estar nas relações
   causais que precisam permanecer válidas durante a transformação, e não em
   uma configuração isolada.

Nenhuma deve ser promovida a definição final antes de experimentos capazes de
fazê-las divergir.

---

## Perturbação e evidência

Remoção continua sendo um operador experimental importante, mas passa a ser
tratada como uma classe possível de perturbação.

Outras perturbações candidatas podem afetar:

```text
estado
estrutura
parâmetros
contexto
evidência
semântica
temporalidade
```

Esses nomes não constituem API nem ontologia adotada.

Também preservamos a distinção:

```text
resiliência do fenômeno
!=
robustez da inferência sobre o fenômeno
```

Perturbar o sistema investiga o fenômeno:

```text
sistema
-> perturbação
-> deslocamento
-> reconfiguração
-> coerência?
```

Perturbar a evidência investiga a inferência:

```text
evidência
-> reamostragem
-> nova inferência
-> mesma conclusão?
```

Essa distinção abre uma ponte futura com reamostragem, suficiência amostral e
ecologia quantitativa sem importar prematuramente um método específico.

---

## Postura experimental

TinyKernel não existe para implementar uma teoria de Kernel já conhecida.

Existe para construir situações pequenas nas quais hipóteses concorrentes
possam produzir previsões diferentes.

Uma comparação futura particularmente relevante é:

```text
ablação
-> witness imediato
```

versus:

```text
ablação
-> oportunidade de reconfiguração
-> trajetória
-> witness
```

Se os procedimentos produzirem conclusões distintas sobre necessidade, essa
diferença será evidência a investigar.

---

## Estado material

TinyKernel permanece em fase **pré-implementação**.

Ainda não existem resultados experimentais próprios nem implementação
substantiva.

O repositório contém atualmente:

```text
README.md
LICENSE
docs/
  TK-FND-00-fundamentos-minimalidade-causal-v0.1.0.md
  notes/
    TK-NOTE-001-ontologia-coerencia-perturbacao-reconfiguracao-resiliencia.md
  source/
    TK-FND-00-fundamentos-minimalidade-causal-v0.1.0.tex
experiments/
  .gitkeep
```

---

## Próximo passo autorizado

O `TK-FND-00` estabelece:

> **documentar TK-0000 antes de implementar TK-0001**

Essa autorização permanece vigente.

`TK-0000` deve provar o **aparato experimental**, não procurar um Kernel
substantivo.

Sua forma fundadora continua sendo:

```text
baseline
-> witness
-> intervention
-> witness
-> classification
```

A `TK-NOTE-001` não altera esse requisito por autoridade própria.

Ao preregistrar `TK-0000`, a relação entre intervenção, mudança, trajetória e
reconfiguração deve ser examinada explicitamente como questão de desenho — não
assumida silenciosamente como resposta.

---

## Documentos

- [TK-FND-00 — Fundamentos para uma Ciência Experimental da Minimalidade Causal](docs/TK-FND-00-fundamentos-minimalidade-causal-v0.1.0.md)
- [TK-NOTE-001 — Ontologia, coerência, perturbação, reconfiguração e resiliência](docs/notes/TK-NOTE-001-ontologia-coerencia-perturbacao-reconfiguracao-resiliencia.md)
- [Fonte LaTeX do TK-FND-00](docs/source/TK-FND-00-fundamentos-minimalidade-causal-v0.1.0.tex)

O `TK-FND-00` preserva o estado fundador.

Notas posteriores registram a evolução conceitual sem reescrever
retroativamente estados anteriores.

---

## READY != COMPLETE

No estágio documental atual, **pronto** significa:

```text
estado versionado
proveniência preservada
documentos reconstruíveis
hipóteses explícitas
limites explícitos
história não reescrita
```

Quando existir aparato executável, `READY` deverá também exigir:

```text
build
execução
verificação
reprodutibilidade
witnesses explícitos
```

**Incompleto** significa que nenhuma realização, ontologia ou definição de
Kernel pode ser universalizada além da evidência que a sustenta.

> **Completude operacional não implica completude ontológica.**

---

## Licença

GNU General Public License v3.0 (`GPL-3.0-only`).

---

> **TinyKernel não procura aquilo que não pode mudar.**
>
> **Procura descobrir o que precisa continuar possível para que algo possa
> mudar sem deixar de ser aquilo que é.**

**Sempre pronto. Sempre incompleto.**
