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
