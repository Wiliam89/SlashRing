&#x20;                                                    ESPECIFICAÇÃO DE PEDAIS E AMP





##### **PreAmp/Amp** (Marshall — Plexi/JCM800/Silver Jubilee)



Referência Slash: Marshall Silver Jubilee 2555 / AFD100 / JCM800 — caráter médio-forte (mid-forward), ganho médio-alto de crunch e lead cantante, não saturação de metal. É a fonte principal de distorção.



Abordagem: cascata de estágios de ganho com waveshaping estilo válvula → tone stack Marshall → power amp.



Preamp: 2–4 estágios de soft clipping assimétrico (ex.: tanh com bias, curvas diferentes por estágio). Entre estágios: HPF de acoplamento (\~10–30 Hz, controla o "aperto" do grave) e um high-cut tipo Miller (\~alguns kHz). Detalhe autêntico: o Jubilee usa um clipper a diodo no canal lead — vale oferecer isso como modo de ganho alto.

Tone stack Marshall (passivo, entre preamp e power amp). Valores de referência pra modelar por transformada bilinear: treble pot 250k, bass 1M, mid 25k; cap treble \~250–500 pF, bass 22 nF, mid 22 nF, slope \~33k. (Variam por modelo — use um "tone stack calculator" como base.)

Power amp: soft clip adicional + presence (shaping de agudo via realimentação negativa) + sag dinâmico (compressão dirigida por envelope, dá o toque responsivo).



Parâmetros (default Slash / faixa):



Gain/Drive: default \~6–7 / 0–10

Bass: \~5 / 0–10 · Mid: \~7 (forte!) / 0–10 · Treble: \~6–7 / 0–10

Presence: \~6 / 0–10

Master (drive do power amp): médio-alto / 0–10

Bright switch: on · Model: Plexi / JCM800 / Jubilee





##### Overdrive (boost na frente — estilo TS/SD-1)



Referência Slash: boost na frente do amp pra empurrá-lo (Boss SD-1 / tube-screamer). Caráter: realce de médios, ganho moderado, aperta o grave. Não é a distorção principal.



Abordagem: HPF pré-clip (tira grave antes de clipar, deixa apertado) → estágio de clipping por diodo (soft) → tone → level.



Pré-clip: o "hump" de médio do TS vem de combinar um caminho plano com o caminho clipado passa-alta em \~720 Hz.

Clipper: soft clip; simétrico (TS) ou assimétrico (SD-1, adiciona harmônicos pares) — deixe como switch.

Saída pode passar de unidade (é o que empurra o amp).



Parâmetros (default Slash / faixa):



Drive: \~2–3 (é boost, não fuzz) / 0–10

Tone: \~5–6 / 0–10

Level: \~7–8 (empurra o amp) / 0–10

Clip mode: TS/SD-1 switch

Tightness (HPF pré-clip): médio / ajustável





##### Delay (analógico/quente — delay de solo)



Referência Slash: delay médio de solo, repetições quentes (agudo cortado no feedback), sutil-a-moderado. Fica depois do cabinet.



Abordagem: linha de delay com interpolação fracionária → feedback com LPF (caráter analógico) → mix. Modulação sutil (wow/flutter BBD) opcional.



Interpolação allpass/Lagrange pra mudar o tempo sem "glitch" de pitch.

Feedback com LPF \~4 kHz (repetições escurecem) e HPF opcional \~100 Hz (evita acúmulo de grave).

Suavize mudança de tempo (crossfade) pra não clicar.



Parâmetros (default Slash / faixa):



Time: \~375–450 ms (semínima em tempos de rock) / 20–2000 ms + sync de BPM

Feedback: \~25–35% (poucas repetições) / 0–100%

Mix: \~20–30% (sutil) / 0–100%

High-cut (repetições): \~4 kHz / ajustável

Mod depth/rate: sutil · Ping-pong: opcional





##### Reverb (sutil — plate/room/spring)



Referência Slash: ambiência discreta, de apoio, não protagonista. Fica depois do cabinet (ordem com o delay é gosto).



Abordagem: reverb algorítmico — Freeverb (8 combs + 4 allpass por canal, leve) ou FDN (4–8 linhas, matriz Hadamard, com modulação pra evitar som metálico). Convolução (IR de sala/plate) se quiser realismo.



Pre-delay antes da rede (preserva o ataque).

Damping = LPF no feedback dos combs (cauda mais escura, senta atrás da guitarra).

HPF na entrada (\~100–200 Hz) pra manter o grave seco e apertado.



Parâmetros (default Slash / faixa):



Type: Plate/Room (Spring pra vibe vintage) / seletor

Mix: \~10–15% (bem sutil) / 0–100%

Decay (RT60): \~1,2–1,8 s / 0,2–10 s

Pre-delay: \~20–40 ms / 0–200 ms

Damping: médio · Size · Width · Low-cut de entrada

Cuidados comuns (valem pros quatro)

Oversampling 4x–8x em todo estágio não-linear (amp e OD). É o que evita o fizz áspero — não compense escurecendo o cabinet.

Sample rate: recompute coeficientes e reinicialize linhas de delay/reverb no prepareToPlay e em mudança de SR.

Denormals: flush-to-zero (FTZ/DAZ) em todo feedback (delay, reverb, filtros). No reverb isso é crítico na cauda.

Suavização de parâmetros (ramps) pra evitar cliques ao girar knobs.

A/B em volume igualado sempre que comparar ligado/desligado.

Cadeia completa: guitar → OD (boost) → PreAmp/Amp → Cabinet (IR) → Delay → Reverb → out.

Como testar (resumo por módulo)

Amp/OD: seno na entrada → cheque o espectro de harmônicos (sem picos de aliasing) e a resposta do tone stack varrendo cada knob.

Delay: meça a precisão do tempo, o decaimento do feedback, confirme que as repetições escurecem e que não clica ao varrer o tempo.

Reverb: meça o RT60, confirme cauda suave (sem flutter metálico), teste a queda até o silêncio sem pico de CPU (denormals).



