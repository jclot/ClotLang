import { source } from '@/lib/source';

export function getLatestDocsVersion(): string {
  const versions = [
    ...new Set(
      source
        .getPages()
        .map((p) => p.slugs[0])
        .filter((s): s is string => /^v\d+\.\d+\.\d+$/.test(s ?? '')),
    ),
  ].sort((a, b) => {
    const parse = (v: string) => v.slice(1).split('.').map(Number) as [number, number, number];
    const [aMaj, aMin, aPatch] = parse(a);
    const [bMaj, bMin, bPatch] = parse(b);
    return bMaj - aMaj || bMin - aMin || bPatch - aPatch;
  });

  return versions[0] ?? 'v0.2.0';
}