# Hook reference

Real Events installs hooks only when the expected original bytes match.

## Town production entry

```text
RVA 0x001101D0
```

Expected original bytes:

```text
83 EC 08 56 57 8B F9
```

The trampoline replays:

```asm
sub esp,08
push esi
push edi
mov edi,ecx
```

and returns to the instruction following the overwritten sequence.

## Town production exits

Three confirmed exits are hooked so the temporary efficiency value can always be restored:

```text
RVA 0x0011077F
RVA 0x001107B4
RVA 0x00110876
```

The common epilogue shape is:

```asm
pop edi
pop esi
add esp,08
ret 4
```

## Private / Trader production entry

```text
RVA 0x000D3798
```

Hook point begins at:

```asm
mov al,[esi+06]
cmp eax,14
```

`ESI` is the private facility entry.

## Private / Trader exit

```text
RVA 0x000D3AE3
```

Observed common exit begins:

```asm
pop edi
pop esi
pop ebp
pop ebx
pop ecx
```

followed by the game's return sequence.

## Tavern Informer

```text
RVA 0x001D1399
```

Expected original bytes:

```text
50 68 BC 66 6C 00
```

Equivalent instructions:

```asm
push eax
push 006C66BC    ; "\f1_%s"
```

At this moment `EAX` points to the complete vanilla Informer text.

Real Events replaces those six bytes with a relative jump, calls its text builder, replays the two original pushes using the resulting pointer, then resumes at:

```text
0x005D139F
```

The custom text buffer is thread-local and currently sized to 4096 bytes.
