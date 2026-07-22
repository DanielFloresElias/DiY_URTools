# Especificaciones del Protocolo de Comunicación DiY_URTools

Este documento detalla el protocolo de intercambio de datos entre el robot Universal Robots (cliente) y la herramienta DiY (servidor TCP) basado en el controlador ESP32.

## 📡 Arquitectura de Red
*   **Protocolo:** TCP/IP Sockets.
*   **Puerto por defecto:** `30002`.
*   **Modo de conexión:** Ciclo de actualización de 100ms (10Hz) para garantizar tiempo real.
*   **Estructura de datos:** Intercambio de tramas fijas de **32 bytes** en ambos sentidos (Full-Duplex).

## 📥 OUT_Buffer (Robot -> Herramienta)
Define las acciones y consignas que el robot envía a la herramienta.

| Byte | Nombre | Descripción | Rango / Valores |
| :--- | :--- | :--- | :--- |
| **0** | **Flags de Acción** | Bits de control (ver detalle abajo) | 8 bits |
| **1** | **Efecto LED** | Tipo de efecto para el anillo luminoso | 0 - 7 |
| **2** | **Consigna (Setpoint)** | Posición deseada de la pinza | 0 (Abierto) - 255 (Cerrado) |
| **3** | **Color 1** | Color principal (ver tabla de colores) | 0 - 15 |
| **4** | **Color 2** | Color secundario para efectos duales | 0 - 15 |
| **5** | **Intervalo** | Velocidad del efecto luminoso | 1 - 255 |

### Detalle de Flags (Byte 0):
*   **Bit 0:** Abrir pinza (Open).
*   **Bit 1:** Cerrar pinza (Close).
*   **Bit 2:** Abrir manual (Override).
*   **Bit 3:** Cerrar manual (Override).

## 📤 IN_Buffer (Herramienta -> Robot)
Contiene la retroalimentación del estado de la herramienta.

| Byte | Nombre | Descripción | Rango / Valores |
| :--- | :--- | :--- | :--- |
| **0** | **Flags de Estado** | Estado de sensores y confirmación | 8 bits |
| **2** | **Posición Actual** | Lectura real del encoder del motor | 0 - 255 |
| **29** | **Modelo** | Identificador del modelo de hardware | ASCII / ID |
| **30-31**| **Firmware Ver.** | Versión del firmware instalada | vX.Y |

## 🎨 Tabla de Colores e Indicadores
El motor de efectos soporta 16 colores predefinidos (Byte 3 y 4), incluyendo:
0: Rojo, 1: Verde, 2: Azul, 3: Amarillo, 4: Magenta, 5: Cian, 8: Blanco, 15: Apagado.

### Efectos Disponibles (Byte 1):
0: Sin efecto, 1: Sólido, 2: Respiración, 3: Knight Rider, 4: Policía, 5: Flash, 6: Arcoíris.
2. Contenido para docs/guia_uso.md
# Guía de Uso e Integración

Esta guía explica cómo poner en marcha las herramientas DiY_URTools e integrarlas en el ecosistema de Universal Robots.

## 🛠️ Requisitos Previos
1.  **Hardware:** PCB DiY_URTools montada con ESP32 y módulo Ethernet W5500.
2.  **Alimentación:** Conector de 12V/24V según el motor utilizado.
3.  **Software:** Robot con PolyScope 5.x o superior.

## ⚙️ Configuración de Red
La herramienta puede configurarse de dos formas:

1.  **Herramienta de Configuración (Python):** Ejecutar `URToolConfigApp` conectando el ESP32 por USB para asignar la IP estática, Máscara y Puerta de enlace.
2.  **Modo Programación:** El firmware incluye un `CONFIGURATION_MODE` que permite el ajuste de parámetros PID (`pinces.setPID(0.4, 0.05, 2.0)`) y límites de fuerza por software para evitar daños en las piezas.

## 🤖 Integración en PolyScope
Para utilizar la herramienta en un programa de robot:

1.  **Instalación de la URCap:** Cargar el archivo `.urcap` generado por el Institut Tecnològic de Barcelona.
2.  **Configuración del Nodo:**
    *   Ir a la pestaña de "Instalación" -> "DiYTool".
    *   Introducir la dirección IP asignada a la herramienta.
    *   Verificar la conexión (el indicador LED de la herramienta debería cambiar de estado).
3.  **Programación:**
    *   Utilizar los nodos de estructura para abrir/cerrar la pinza.
    *   Configurar los efectos visuales para indicar estados (ej: Rojo = Error, Verde = Operación normal).

## ⚠️ Mantenimiento y Seguridad
*   **Detección de pieza:** El sistema monitoriza el consumo de corriente. Si la corriente supera el `gripCurrentThreshold`, la pinza detecta que ha sujetado un objeto y detiene el movimiento.
*   **Watchdog:** El firmware incluye un sistema de seguridad que detiene los motores si se pierde la comunicación TCP con el robot por más de 500ms.