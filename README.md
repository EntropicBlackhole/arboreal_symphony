# Arboreal Symphony: Neural-Symbolic Music Generator

## English

**Arboreal Symphony** is a high-performance, neural-symbolic music generation engine built in C++. Unlike heavy deep-learning models, it utilizes a Mixture of Experts (MoE) architecture driven by **Boolean Decision Trees** and **Genetic Algorithms (GA)** to compose structural music that mimics the complexity of human-arranged themes.

### How it Works

1. **Data Factory (Python):** Ingests MIDI corpora to calculate transition probability matrices (Markov Chains).
2. **Genetic Engine (C++):** An evolutionary core optimizes trees by maximizing a hybrid fitness function (Markov statistical evaluation + Boolean counterpoint penalties + Levenshtein distance for motif memory).
3. **The Maestro (Orchestrator):** Implements a continuous 80-beat assembly arc with native ADSR energy envelopes, modulating keys dynamically and injecting human-like acoustic physics (Rubato Jitter & Sustain Pedal resonance).

### Usage

Ensure you have `g++` and Python 3 installed.

**0. Compliling**
To compile it from scratch:

```bash
g++ -O3 -march=native -ffast-math -std=c++17 main.cpp -o train_music

```

**1. Training**
To train a model from scratch on a specific theme:

```bash
./train_music --pop=1000 --gens=1000 --theme=The_Hollow_Knight_Duel --seed=duel

```

**2. Generation**
To generate a new symphony from a trained model using a semantic key:

```bash
./generate <RunID> cinematic

```

---

## Español

**Arboreal Symphony** es un motor de generación musical simbólica de alto rendimiento, desarrollado en C++. A diferencia de los modelos pesados de *deep learning*, utiliza una arquitectura de *Mixture of Experts* (MoE) impulsada por **Árboles de Decisión Booleanos** y **Algoritmos Genéticos (GA)** para componer música estructural con la complejidad de arreglos humanos.

### Cómo funciona

1. **Fábrica de Datos (Python):** Ingiere corpus MIDI para calcular matrices de probabilidad de transición (Cadenas de Markov).
2. **Motor Genético (C++):** Un núcleo evolutivo optimiza los árboles maximizando una función de aptitud híbrida (evaluación estadística de Markov + penalizaciones de contrapunto booleano + distancia de Levenshtein para memoria de motivos).
3. **El Maestro (Orquestador):** Implementa un arco de ensamble continuo de 80 compases con envolventes de energía ADSR, modulando tonalidades dinámicamente e inyectando físicas acústicas nativas (Jitter de Rubato y resonancia de Pedal de Sustain).

### Uso

Asegúrate de tener instalado `g++` y Python 3.

**0. Complicacion**
Para compilarlo desde 0:

```bash
g++ -O3 -march=native -ffast-math -std=c++17 main.cpp -o train_music

```

**1. Entrenamiento**
Para entrenar un modelo desde cero en un tema específico:

```bash
./train_music --pop=1000 --gens=1000 --theme=The_Hollow_Knight_Duel --seed=duel

```

**2. Generación**
Para generar una nueva sinfonía desde un modelo entrenado usando una clave semántica:

```bash
./generate <RunID> cinematic

```

---

*README.md generado con Gemini*
