import Link from 'next/link';
import { ArrowRightIcon, BookOpenIcon, GithubIcon } from 'lucide-react';
import { getLatestDocsVersion } from '@/lib/docs-versions';
import { gitConfig } from '@/lib/layout.shared';
import { HeroShowcase } from '@/components/home/hero-showcase';
import { ModeTabs } from '@/components/home/mode-tabs';
import { FeatureBento } from '@/components/home/feature-bento';

const GRADIENT = 'from-fuchsia-500 via-violet-500 to-sky-500';

export default function HomePage() {
  const v = getLatestDocsVersion();
  const repo = `https://github.com/${gitConfig.user}/${gitConfig.repo}`;
  const installCmd =
    'curl -fsSL https://raw.githubusercontent.com/jclot/ClotLang/master/scripts/install.sh | bash';

  return (
    <main className="relative flex flex-1 flex-col">
      {/* ══ HERO ══════════════════════════════════════════════════════ */}
      <section className="mx-auto w-full max-w-6xl px-4 pt-8 sm:pt-12">
        <div className="relative overflow-hidden rounded-3xl border border-fd-border bg-fd-card">
          {/* Background layers */}
          <div
            aria-hidden
            className="pointer-events-none absolute inset-0 z-0"
          >
            {/* animated aurora blobs (ClotLang droplet palette) */}
            <div
              className="clot-aurora absolute -left-24 -top-28 size-[460px] rounded-full blur-3xl"
              style={{
                background:
                  'radial-gradient(circle, rgba(217,70,239,0.38), transparent 70%)',
              }}
            />
            <div
              className="clot-aurora absolute -right-16 -top-20 size-[420px] rounded-full blur-3xl"
              style={{
                animationDelay: '-7s',
                background:
                  'radial-gradient(circle, rgba(56,189,248,0.34), transparent 70%)',
              }}
            />
            <div
              className="clot-aurora absolute left-1/3 top-8 size-[400px] rounded-full blur-3xl"
              style={{
                animationDelay: '-13s',
                background:
                  'radial-gradient(circle, rgba(139,92,246,0.32), transparent 70%)',
              }}
            />
            {/* dot grid */}
            <div
              className="absolute inset-0 opacity-[0.6]"
              style={{
                backgroundImage:
                  'radial-gradient(color-mix(in oklch, var(--color-fd-border) 90%, transparent) 1px, transparent 1px)',
                backgroundSize: '22px 22px',
                maskImage:
                  'radial-gradient(ellipse 80% 80% at 50% 0%, black 20%, transparent 80%)',
                WebkitMaskImage:
                  'radial-gradient(ellipse 80% 80% at 50% 0%, black 20%, transparent 80%)',
              }}
            />
            {/* film grain */}
            <div className="clot-grain absolute inset-0 opacity-[0.12] mix-blend-overlay" />
          </div>

          <div className="relative z-10 grid grid-cols-1 items-center gap-10 px-6 py-14 sm:px-10 sm:py-16 lg:grid-cols-[1.05fr_1fr] lg:gap-8 lg:py-20">
            {/* Left: copy */}
            <div className="text-center lg:text-left">
              <span className="inline-flex items-center gap-2 rounded-full border border-fd-border bg-fd-background/70 px-3 py-1 text-xs font-medium text-fd-muted-foreground backdrop-blur">
                <span
                  className={`size-1.5 rounded-full bg-gradient-to-r ${GRADIENT}`}
                />
                Interpreted &amp; compiled · v0.3.4
              </span>

              <h1 className="mt-5 text-balance text-4xl font-bold tracking-tight sm:text-6xl">
                Write once.
                <br />
                <span
                  className={`clot-animated-text bg-gradient-to-r ${GRADIENT} bg-clip-text text-transparent`}
                >
                  Run or compile.
                </span>
              </h1>

              <p className="mx-auto mt-6 max-w-xl text-balance text-lg text-fd-muted-foreground lg:mx-0">
                ClotLang runs instantly through its interpreter and compiles to
                native binaries with LLVM — gradual typing, first-class OOP, and
                bilingual diagnostics, all from a single toolchain.
              </p>

              <div className="mt-8 flex flex-wrap items-center justify-center gap-3 lg:justify-start">
                <Link
                  href={`/docs/${v}/installation`}
                  className={`inline-flex h-11 items-center gap-2 rounded-lg bg-gradient-to-r ${GRADIENT} px-5 text-sm font-semibold text-white shadow-lg shadow-fd-primary/20 transition-transform hover:scale-[1.03]`}
                >
                  Get started
                  <ArrowRightIcon className="size-4" />
                </Link>
                <Link
                  href={`/docs/${v}`}
                  className="inline-flex h-11 items-center gap-2 rounded-lg border border-fd-border bg-fd-background/60 px-5 text-sm font-semibold text-fd-foreground backdrop-blur transition-colors hover:bg-fd-accent"
                >
                  <BookOpenIcon className="size-4" />
                  Read the docs
                </Link>
                <a
                  href={repo}
                  target="_blank"
                  rel="noreferrer"
                  className="inline-flex size-11 items-center justify-center rounded-lg border border-fd-border bg-fd-background/60 text-fd-foreground backdrop-blur transition-colors hover:bg-fd-accent"
                  aria-label="GitHub repository"
                >
                  <GithubIcon className="size-4" />
                </a>
              </div>

              <div className="mx-auto mt-6 flex max-w-xl items-center gap-3 overflow-x-auto rounded-lg border border-fd-border bg-fd-background/60 px-4 py-3 font-mono text-xs text-fd-foreground backdrop-blur lg:mx-0">
                <span className="select-none text-fd-primary">$</span>
                <code className="whitespace-nowrap">{installCmd}</code>
              </div>
            </div>

            {/* Right: animated showcase */}
            <div className="flex justify-center lg:justify-end">
              <HeroShowcase />
            </div>
          </div>
        </div>
      </section>

      {/* ══ MARQUEE ═══════════════════════════════════════════════════ */}
      <section className="relative mx-auto mt-12 w-full max-w-full overflow-hidden py-2">
        <div className="clot-marquee flex w-max gap-3">
          {[...MARQUEE, ...MARQUEE].map((item, i) => (
            <span
              key={i}
              className="inline-flex shrink-0 items-center rounded-full border border-fd-border bg-fd-card px-4 py-1.5 font-mono text-sm text-fd-muted-foreground"
            >
              {item}
            </span>
          ))}
        </div>
        <div className="pointer-events-none absolute inset-y-0 left-0 w-24 bg-gradient-to-r from-fd-background to-transparent" />
        <div className="pointer-events-none absolute inset-y-0 right-0 w-24 bg-gradient-to-l from-fd-background to-transparent" />
      </section>

      {/* ══ EXECUTION MODES ═══════════════════════════════════════════ */}
      <section className="mx-auto w-full max-w-6xl px-4 pt-24">
        <div className="mb-10 text-center">
          <p className="mb-2 text-sm font-medium text-fd-primary">
            One source, three superpowers
          </p>
          <h2 className="text-3xl font-bold tracking-tight sm:text-4xl">
            Pick the mode that fits the{' '}
            <span
              className={`bg-gradient-to-r ${GRADIENT} bg-clip-text text-transparent`}
            >
              moment
            </span>
          </h2>
        </div>
        <ModeTabs />
      </section>

      {/* ══ FEATURES ══════════════════════════════════════════════════ */}
      <section className="mx-auto w-full max-w-6xl px-4 pt-24">
        <div className="mb-10 text-center">
          <p className="mb-2 text-sm font-medium text-fd-primary">
            Small language, real range
          </p>
          <h2 className="text-3xl font-bold tracking-tight sm:text-4xl">
            Features that pull their{' '}
            <span
              className={`bg-gradient-to-r ${GRADIENT} bg-clip-text text-transparent`}
            >
              weight
            </span>
          </h2>
        </div>

        <FeatureBento />
      </section>

      {/* ══ CTA ═══════════════════════════════════════════════════════ */}
      <section className="mx-auto w-full max-w-6xl px-4 py-24">
        <div className="relative overflow-hidden rounded-3xl border border-fd-border bg-fd-card px-6 py-16 text-center">
          <div aria-hidden className="pointer-events-none absolute inset-0 z-0">
            <div
              className="clot-aurora absolute left-1/4 -top-24 size-[380px] rounded-full blur-3xl"
              style={{
                background:
                  'radial-gradient(circle, rgba(217,70,239,0.3), transparent 70%)',
              }}
            />
            <div
              className="clot-aurora absolute right-1/4 -top-16 size-[340px] rounded-full blur-3xl"
              style={{
                animationDelay: '-9s',
                background:
                  'radial-gradient(circle, rgba(56,189,248,0.28), transparent 70%)',
              }}
            />
            <div className="clot-grain absolute inset-0 opacity-[0.1] mix-blend-overlay" />
          </div>

          <h2 className="relative z-10 text-3xl font-bold tracking-tight sm:text-4xl">
            Ready to write your first program?
          </h2>
          <p className="relative z-10 mx-auto mt-4 max-w-md text-fd-muted-foreground">
            Install the toolchain in a single command and run a .clot file in
            minutes.
          </p>
          <div className="relative z-10 mt-9 flex flex-wrap items-center justify-center gap-3">
            <Link
              href={`/docs/${v}/installation`}
              className={`inline-flex h-11 items-center gap-2 rounded-lg bg-gradient-to-r ${GRADIENT} px-5 text-sm font-semibold text-white shadow-lg shadow-fd-primary/20 transition-transform hover:scale-[1.03]`}
            >
              Install ClotLang
              <ArrowRightIcon className="size-4" />
            </Link>
            <Link
              href={`/docs/${v}/first_run`}
              className="inline-flex h-11 items-center gap-2 rounded-lg border border-fd-border bg-fd-background/60 px-5 text-sm font-semibold text-fd-foreground backdrop-blur transition-colors hover:bg-fd-accent"
            >
              First run
            </Link>
          </div>
        </div>
      </section>

      {/* ══ FOOTER ════════════════════════════════════════════════════ */}
      <footer className="border-t border-fd-border">
        <div className="mx-auto flex w-full max-w-6xl flex-col items-center justify-between gap-4 px-4 py-8 text-sm text-fd-muted-foreground sm:flex-row">
          <p>© {new Date().getFullYear()} ClotLang. Open source.</p>
          <nav className="flex flex-wrap items-center justify-center gap-x-6 gap-y-2">
            <Link href={`/docs/${v}`} className="transition-colors hover:text-fd-foreground">
              Documentation
            </Link>
            <Link href="/blog" className="transition-colors hover:text-fd-foreground">
              Blog
            </Link>
            <Link href={`/docs/${v}/installation`} className="transition-colors hover:text-fd-foreground">
              Install
            </Link>
            <a href={repo} target="_blank" rel="noreferrer" className="transition-colors hover:text-fd-foreground">
              GitHub
            </a>
          </nav>
        </div>
      </footer>
    </main>
  );
}

const MARQUEE = [
  'println(...)',
  'for i in range(n): … endfor',
  '--mode compile',
  'abstract class',
  'gradual typing',
  'clot.science.calculus.dual',
  'try / catch / finally',
  '--lang es',
  'defer',
  'interfaces',
  'string interpolation',
  '--emit exe',
];
