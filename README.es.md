# Real Events para Patrician III

 <img src="docs/images/logo.png" alt="Patrician 3 - Real Events" width="360">

**Versión:** 0.1.0  
**Estado:** primera candidata a versión pública

Real Events añade eventos económicos dinámicos a *Patrician III*. Las ciudades pueden sufrir crisis temporales de producción o experimentar periodos de auge. Los efectos se aplican tanto a la producción de la ciudad como a las empresas privadas, y los Informers de la taberna pueden dar pistas sobre los eventos activos.

## Funciones de v0.1.0

- Entre 1 y 5 ciudades con eventos activos.
- **Crisis:** producción aproximada del 40–60%.
- **Boom:** producción aproximada del 140–160%.
- Duración aleatoria de 1 a 12 meses de juego.
- Evaluación de nuevos eventos cada 30–90 días de juego.
- Persistencia independiente por campaña.
- Un rumor adicional de Real Events en la conversación del Informer.
- El texto vanilla del Informer se conserva; el mod añade su rumor al final.
- Las herramientas F10/F11/debug quedan desactivadas en la compilación pública normal.

## Instalación prevista

Real Events está probado con la versión GOG de *Patrician III* y el ejecutable de 32 bits utilizado por el modloader de P3Modding.

La DLL pública será:

```text
RealEvents.dll
```

y deberá copiarse en la carpeta `mods` del modloader de P3Modding.

El flujo esperado es el mismo que documenta P3Modding para su modloader:

1. Colocar `p3_modloader.dll` y `Patrician3_modloader.exe` en la carpeta de *Patrician III*.
2. Crear una carpeta `mods`.
3. Copiar las DLLs de los mods dentro de `mods`.
4. Iniciar el juego con `Patrician3_modloader.exe`.

Referencia: https://p3modding.github.io/modloader.html

Otras versiones del ejecutable del juego podrían funcionar, pero v0.1.0 solo se ha validado con la versión GOG.

## Estado de la configuración

En v0.1.0 los parámetros están todavía compilados en la DLL. No se incluye un `RealEvents.ini` funcional todavía para no introducir cambios no probados en la primera release candidate. Será una de las primeras mejoras después de estabilizar esta versión.

## Documentación técnica

Consulta `docs/reverse-engineering.md`, `docs/memory-layout.md` y `docs/hooks.md`.

La documentación diferencia siempre entre información ya existente en P3Modding/comunidad y descubrimientos confirmados mediante nuestras propias pruebas.

## Capturas

| Crisis de producción | Boom de producción |
| --- | --- |
| <img src="docs/images/informer_crisis.png" alt="Rumor del Informer para un evento de crisis de producción" width="360"> | <img src="docs/images/informer_boom.png" alt="Rumor del Informer para un evento de boom de producción" width="360"> |

## Licencia

Publicado bajo la Apache License, Version 2.0. Consulta `LICENSE`.
