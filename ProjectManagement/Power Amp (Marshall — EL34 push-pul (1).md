##### Power Amp (Marshall — EL34 push-pull, Classe AB)



Referência Slash: seção de potência EL34 push-pull, Classe AB, com malha de realimentação negativa (é dela que saem Presence e Resonance). Retificador solid-state (Jubilee) = mais apertado/menos sag; valvulado (Plexi) = mais sag/esponjoso.



1\. Conceito (o que ele faz, além do preamp)

Saturação de válvula de potência quando empurrado (Master alto) — caráter harmônico diferente do preamp (mais compressão, distorção de crossover da Classe AB).

Sag: queda dinâmica de "tensão" sob sinal forte → compressão/bloom nos transientes. É o que dá o toque responsivo.

Presence: realce de agudo reduzindo a realimentação negativa nos highs.

Resonance (Depth): realce de grave (o thump) reduzindo a realimentação nos lows.

Interação com o cabinet: o trafo de saída + impedância do alto-falante criam uma resposta dependente de frequência. Cuidado: se você já usa IR do cab, ela já captura o alto-falante — então aqui isso deve ser sutil (um controle de "resonance"), pra não duplicar o cabinet.

2\. Abordagem (DSP)

Phase inverter → par push-pull modelado como dois waveshapers (ou um soft clip com componente de crossover pra Classe AB): distorção de crossover em nível baixo, compressão perto do clip.

Malha de realimentação: caminho de feedback com ganho dependente de frequência. Presence reduz o NFB nos highs, Resonance reduz nos lows. Pode implementar como filtros de pré/de-ênfase em volta do clipper (mais simples) ou como feedback real com filtro no retorno (mais autêntico, cuidado com estabilidade).

Sag: envelope follower reduz o headroom/ganho sob sustain e recupera com uma constante de tempo — o tipo de retificador define quantidade e velocidade.

Oversampling 4x–8x em volta da não-linearidade.

3\. Parâmetros (default Slash / faixa)

Master / Power (drive nas válvulas de potência): médio-alto / 0–10

Presence: \~6 / 0–10 (agudo via NFB)

Resonance (Depth): \~5–6 / 0–10 (grave/thump)

Sag: médio / 0–10 — ou switch retificador solid-state (tight) / valvulado (spongy)

Tube type: EL34 (Marshall) / 6L6 (voz alternativa)

4\. Cuidados

Não duplique o cabinet: com IR ativa, a interação trafo/alto-falante deve ser leve (só o "resonance"), não uma segunda camada de filtragem.

Estabilidade se modelar o feedback de verdade (Presence/Resonance mexem na malha).

Denormals (FTZ) no envelope do sag e no feedback.

Gain staging: saída do preamp → power amp → cab, tudo em nível coerente.

5\. Como testar

Varredura senoidal: confirme que Presence levanta o agudo e Resonance levanta o grave.

Teste dinâmico (sag): ataque forte → deve haver compressão/bloom e depois recuperação; compare solid-state vs valvulado.

Espectro em Master alto: cheque o caráter da distorção de potência (crossover em nível baixo) sem picos de aliasing.

A/B preamp-only vs preamp+power amp em volume igualado: o power amp deve adicionar compressão e feel, não só volume.



Cadeia atualizada: guitar → OD (boost) → PreAmp → PowerAmp → Cabinet (IR) → Delay → Reverb → out.




separe internamente em dois módulos (PreAmp e PowerAmp), mas agrupe-os como um só "Amp" na interface. É o melhor dos dois mundos.

Por que separar no código/DSP:

São DSPs diferentes. Preamp = estágios de ganho em cascata + tone stack. Power amp = push-pull + sag + presence/resonance via realimentação. Misturar tudo numa classe só vira uma bagunça difícil de manter.
Testabilidade — você consegue validar cada um isolado (era exatamente o A/B "preamp-only vs preamp+power amp" que citei). Módulos separados = teste separado.
Combina com o seu fluxo de trabalho: você está fazendo uma spec por módulo, um chat por módulo, verificação por módulo. Separar mantém essa disciplina — o programador recebe uma responsabilidade clara por vez e "inventa" menos.
Flexibilidade: se um dia quiser inserir algo entre eles (um loop de efeitos, ou testar a posição do tone stack), é trivial.

Por que agrupar na interface/mental:

Pro usuário, "Amp" é uma coisa só com knobs (Gain, Bass/Mid/Treble, Presence, Master). Presence e Master são funções do power amp, mas ninguém pensa nelas como "outro aparelho". Um painel único "Amp" é mais intuitivo que dois separados.

Ou seja: duas classes encadeadas por dentro (PreAmp → PowerAmp), um painel "Amp" por fora.

Um detalhe prático de implementação: envolva os dois estágios não-lineares num único bloco de oversampling (sobe a taxa uma vez, processa preamp + power amp, desce uma vez). Assim você não paga o custo de reamostrar duas vezes, e o tone stack (que é linear) no meio não sofre com isso. Só evite deixar cada módulo fazendo o próprio up/downsample — desperdiça CPU e pode acumular artefato.

Resumindo: separados por dentro (limpo, testável, alinhado ao seu processo), unidos por fora (intuitivo pro usuário), com um oversampling compartilhado envolvendo a dupla.