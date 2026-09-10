# TK-0001 — Resultado

Status: `REPRODUCED`

Foram executados três runs e duas trajetórias a partir da baseline:

- baseline: `PRESERVED`;
- `replace feedback feedback_equivalent`: `PRESERVED`;
- `remove update`: `BROKEN_CAUSAL`.

Os cinco witnesses por run produziram 15 observações e 15 registros de evidência
imutável. O claim de suficiência relativa L2 e o claim de necessidade relativa L3
foram sustentados. O claim de minimalidade relativa L5 permaneceu `open`.

Exemplos de digests de evidência:

```text
TK-0001:RUN:BASELINE:E:operational
1005dcc2d4b58688b799bc1baf21f359bd0f457726883650d1c935ae74547303

TK-0001:RUN:REPLACE_FEEDBACK:E:temporal
fe6359591baf92da68f071a0e25eb89af4961cdc40c62b84c969cc51fbb2fd86

TK-0001:RUN:REMOVE_UPDATE:E:causal
940075f8a6fff11139b992a9471776abb18049b4cd946f22158b6bba140c420e
```

Digest SHA-256 do export canônico seguido de newline:

```text
511c4810c76dc53cd9363ab7c2b0feae3feb80e3af678f0d2c815481b8021f55
```

Limite: três intervenções permanecem não exploradas; o espaço não sustenta L4–L8
nem qualquer declaração de minimalidade global.
