
```markdown
# 🐾 Clyvo Pet - Guia de Avaliação (Challenge FIAP 2026)

**Disciplina:** DISRUPTIVE ARCHITECTURES: IOT, IOB & GENERATIVE IA  
**Empresa Parceira:** Clyvo Vet  
**Equipe:**
* Emanuel Italo Leal Trindade Soares (RM561337)
* Paulo Henrique Alves Estalise (RM563811)
* Gabriel Bebe (RM562012)

---

## 📌 Sobre o Projeto link github ( https://github.com/Emanuel-italo/Challenge-IOT )

Este repositório contém a entrega das **Sprints 1 e 2** referentes ao desafio de criar a infraestrutura do futuro da medicina veterinária digital.

A **Clyvo Pet** é uma solução IoT focada em resolver a fragmentação da jornada de saúde do animal. O projeto consiste em uma simulação de um *wearable* (coleira inteligente) conectado em tempo real a duas interfaces distintas: um **Aplicativo do Tutor** e um **Dashboard Clínico (Admin)**.

### 💡 O Diferencial Técnico: Motor Fisiológico Dinâmico
Para atender aos requisitos de avaliação de forma realista, **não utilizamos dados de sensores estáticos (mockados)**. Desenvolvemos no ESP32 (C++) um "Motor Fisiológico". O algoritmo reage de forma autônoma: se o pet está em repouso, os sinais vitais caem e estabilizam; se entra em atividade, o Arduino simula progressivamente o aumento dos batimentos cardíacos (BPM), a elevação da temperatura corporal e o ganho de distância percorrida.

---

## 📂 Estrutura do Projeto e Código-Fonte

```text
📦 challenge-iot-clyvo
 ┣ 📂 Firmware
 ┃ ┗ 📜 main.ino          # Código C++ embarcado para o ESP32 (Motor Fisiológico)
 ┣ 📂 Dashboards
 ┃ ┣ 📜 app_tutor.html    # Interface Mobile-first para o dono do pet
 ┃ ┗ 📜 ADM.html          # Painel de Triagem e Monitoramento para a Clínica
 ┣ 📂 Json
 ┃  ┗ 📜 para_inserir_no_projeto.json #para inserir no projeto
 ┗ 📜 README.md           # Documentação principal

```

---

## ⚙️ Tecnologias Utilizadas

| Componente | Tecnologia / Ferramenta | Descrição |
| --- | --- | --- |
| **Hardware IoT** | ESP32 (C++) | Microcontrolador simulado no **Wokwi** com Arduino Framework. |
| **Protocolo** | MQTT (HiveMQ) | Comunicação assíncrona bidirecional em tempo real de baixa latência. |
| **Front-end** | HTML5, CSS3, JS Vanilla | Construção das interfaces limpas e responsivas. |
| **Bibliotecas** | `PubSubClient`, `Chart.js` | Conexão IoT via WebSockets e renderização de gráficos clínicos em tela. |

---

## 🚀 Como Testar o Projeto (Guia passo a passo)

O sistema foi desenhado para funcionar em nuvem, sem necessidade de configurar servidores locais.

### Passo 1: Inicializar o Dispositivo (ESP32)

1. Acesse o projeto no simulador **Wokwi**.
2. Certifique-se de que o código `main.ino` (presente na pasta *Firmware*) está no editor.
3. Clique no botão de **Play (Start Simulation)**.
4. Aguarde o terminal Serial exibir a mensagem: `Broker MQTT Conectado`. Neste momento, o ESP32 começará a transmitir os dados basais (estado de repouso).

### Passo 2: Inicializar as Interfaces (Dashboards)

1. Baixe os arquivos da pasta *Dashboards* para o seu computador.
2. Abra o arquivo `app_tutor.html` em uma aba do navegador *(visão do dono do pet)*.
3. Abra o arquivo `ADM.html` em outra aba ou janela *(visão do veterinário na clínica)*.
4. Ambos os painéis exibirão "Broker Conectado" e começarão a receber os dados do Wokwi. O status inicial será **VERDE (Quadro Estável / Repouso)**.

### Passo 3: Simulando a Mudança de Comportamento (A Mágica)

Para avaliar a interatividade e a inteligência do sistema:

1. Vá até a tela do **App do Tutor**, role até a seção de atividade e clique em **"▶️ INICIAR ATIVIDADE"**.
2. **Observe o comportamento nos 3 ambientes simultaneamente:**
* **No Wokwi (Serial):** O log mostrará `>>> INICIO DE ATIVIDADE <<<`. Os valores de BPM, Temperatura e Distância começarão a subir progressivamente.
* **No App do Tutor:** A quilometragem começará a rodar, e o banner mudará para **AMARELO (Em atividade)**.
* **No Dashboard Clínico (ADM):** O gráfico de temperatura começará a registrar uma curva ascendente. O painel acusará "Quadro de Esforço".


3. **O Alerta Crítico:** Deixe a atividade rodando por alguns segundos. Quando a temperatura simulada pelo ESP32 ultrapassar **39.5 °C** ou o BPM passar de **150**, o sistema entrará em colapso biológico: o status ficará **VERMELHO**, o *Buzzer* apitará no Wokwi e um alerta de Emergência será registrado no painel do veterinário.
4. Volte ao App do Tutor e clique em **"⏹️ FINALIZAR ATIVIDADE"** para observar o pet "esfriar" e os sinais vitais retornarem ao normal de forma gradativa.

---

## 📊 Resultados Parciais (Sprint 1 e 2)

Nesta fase de entrega, o projeto validou com sucesso:

* **Comunicação de Baixa Latência:** Implementação da arquitetura Pub/Sub garantindo que os alertas da coleira cheguem instantaneamente à tela da clínica.
* **Inteligência Embarcada:** Conclusão do "Motor Fisiológico" em C++ que elimina o uso de variáveis estáticas, gerando simulações reais de repouso, exercício e risco de taquicardia/febre.
* **Interface Dual-View:** Separação das jornadas e regras de negócio, provando o conceito de continuidade do cuidado exigido pela Clyvo Vet.

---

## ✅ Requisitos do Challenge Atendidos

* **IoT e Wearables:** Protótipo construído em C++ (ESP32).
* **Protocolos:** Integração completa utilizando MQTT.
* **Dashboard:** Criação de painéis web consumindo dados de telemetria em tempo real.
* **Aderência ao Negócio:** Conexão direta entre o momento do passeio do pet em casa com a triagem médica preventiva na clínica.

```