# Real Events para Patrician III

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

La DLL pública será:

```text
RealEvents.dll
```

y deberá copiarse en la carpeta `mods` del modloader de P3Modding.

## Estado de la configuración

En v0.1.0 los parámetros están todavía compilados en la DLL. No se incluye un `RealEvents.ini` funcional todavía para no introducir cambios no probados en la primera release candidate. Será una de las primeras mejoras después de estabilizar esta versión.

## Documentación técnica

Consulta `docs/reverse-engineering.md`, `docs/memory-layout.md` y `docs/hooks.md`.

La documentación diferencia siempre entre información ya existente en P3Modding/comunidad y descubrimientos confirmados mediante nuestras propias pruebas.
