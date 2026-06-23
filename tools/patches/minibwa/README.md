# minibwa patch queue

Patches in this directory are applied in lexical order by:

```sh
tools/apply-minibwa-patches.sh .sync/minibwa
Rscript tools/vendor-minibwa.R unpack
```

The upstream version is pinned in `tools/minibwa-upstream.dcf`. Keep patches
small and source-compatible with that exact commit. When bumping upstream:

1. update the DCF lock file;
2. refresh the patch queue against the new commit;
3. run `tools/build-minibwa-cli.sh`;
4. run the chrM smoke test and record any behavior changes.

## Current patches

- `0001-ksw-use-simde-include-layer.patch`: add an opt-in
  `RMINIBWA_USE_SIMDE` include path for the KSW SSE kernels. Upstream behavior
  is unchanged unless the macro is defined.
- `0002-fix-checked-reads-and-subbatch-memset.patch`: add checked binary reads
  for index loading and make the sub-batch `memset()` size expression explicit.
- `0003-add-avx2-wide-extd2-kernel.patch`: add the widened AVX2 dual-gap
  `ksw_extd2` kernel used by the staged `avx2` backend. This patch is derived
  from the local `fg-labs/minibwa-bindings` reconnaissance copy and is kept in
  the patch queue rather than treated as upstream minibwa source.
