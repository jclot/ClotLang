'use client';

import { useState } from 'react';
import {
  BoxIcon,
  LanguagesIcon,
  LayersIcon,
  SparklesIcon,
  WandSparklesIcon,
} from 'lucide-react';
import { SpotlightCard } from './spotlight-card';

/* ── token helpers ──────────────────────────────────────────────────── */
const fn = 'text-fd-primary';
const num = 'text-amber-600 dark:text-amber-400';
const kw = 'text-violet-600 dark:text-violet-400';
const ty = 'text-sky-600 dark:text-sky-400';
const cm = 'text-fd-muted-foreground/70';
const pn = 'text-fd-muted-foreground';

function CardHead({
  icon: Icon,
  title,
  children,
}: {
  icon: React.ElementType;
  title: string;
  children: React.ReactNode;
}) {
  return (
    <>
      <div className="mb-3 flex size-10 items-center justify-center rounded-lg bg-fd-primary/10 text-fd-primary ring-1 ring-fd-primary/15 transition-transform group-hover:scale-105">
        <Icon className="size-5" />
      </div>
      <h3 className="font-semibold text-fd-foreground">{title}</h3>
      <p className="mt-1.5 text-sm leading-relaxed text-fd-muted-foreground">
        {children}
      </p>
    </>
  );
}

const codeBox =
  'mt-4 overflow-x-auto rounded-lg border border-fd-border bg-fd-muted/40 p-3 font-mono text-xs leading-relaxed';

const DIAG = {
  es: {
    msg: " no se puede instanciar la clase abstracta 'Shape'",
    help: '  ayuda: implementa area() en una subclase concreta',
  },
  en: {
    msg: " cannot instantiate abstract class 'Shape'",
    help: '  help: implement area() in a concrete subclass',
  },
};

export function FeatureBento() {
  const [lang, setLang] = useState<'es' | 'en'>('es');
  const d = DIAG[lang];

  return (
    <div className="grid grid-cols-1 gap-4 sm:grid-cols-2 lg:grid-cols-6">
      {/* ── Row 1 · three code cards ─────────────────────────────────── */}
      <SpotlightCard className="lg:col-span-2">
        <div className="flex h-full flex-col p-6">
          <CardHead icon={LayersIcon} title="Gradual typing">
            Start untyped, add annotations where they earn their keep.
          </CardHead>
          <pre className={`${codeBox} mt-auto`}>
            <code>
              <span className={pn}>x = </span>
              <span className={num}>3</span>
              <span className={pn}>; </span>
              <span className={cm}> // untyped</span>
              {'\n\n'}
              <span className={kw}>func </span>
              <span className={ty}>Int </span>
              <span className={fn}>add</span>
              <span className={pn}>(a: </span>
              <span className={ty}>Int</span>
              <span className={pn}>, b: </span>
              <span className={ty}>Int</span>
              <span className={pn}>):</span>
            </code>
          </pre>
        </div>
      </SpotlightCard>

      <SpotlightCard className="lg:col-span-2">
        <div className="flex h-full flex-col p-6">
          <CardHead icon={BoxIcon} title="First-class OOP">
            Classes, interfaces, inheritance — and now abstract members.
          </CardHead>
          <pre className={`${codeBox} mt-auto`}>
            <code>
              <span className={kw}>abstract class </span>
              <span className={ty}>Shape</span>
              <span className={pn}>:</span>
              {'\n    '}
              <span className={kw}>abstract func </span>
              <span className={ty}>Float </span>
              <span className={fn}>area</span>
              <span className={pn}>():</span>
              {'\n'}
              <span className={kw}>endclass</span>
            </code>
          </pre>
        </div>
      </SpotlightCard>

      <SpotlightCard className="lg:col-span-2">
        <div className="flex h-full flex-col p-6">
          <CardHead icon={SparklesIcon} title="Batteries included">
            A growing stdlib — down to automatic differentiation.
          </CardHead>
          <pre className={`${codeBox} mt-auto`}>
            <code>
              <span className={pn}>d = </span>
              <span className={fn}>differentiate</span>
              <span className={pn}>(f);</span>
              {'\n'}
              <span className={pn}>d.</span>
              <span className={fn}>at</span>
              <span className={pn}>(</span>
              <span className={num}>2.0</span>
              <span className={pn}>); </span>
              <span className={cm}> // 4.0</span>
            </code>
          </pre>
        </div>
      </SpotlightCard>

      {/* ── Row 2 · two wide cards ───────────────────────────────────── */}
      <SpotlightCard className="sm:col-span-2 lg:col-span-3">
        <div className="flex h-full flex-col p-6">
          <CardHead icon={LanguagesIcon} title="Diagnostics in your language">
            Every error and CLI string is localized. Flip{' '}
            <code className="font-mono text-fd-foreground">--lang</code> and the
            whole compiler speaks English or Spanish.
          </CardHead>

          <div className="mt-4 overflow-hidden rounded-lg border border-fd-border bg-fd-muted/40 font-mono text-xs">
            <div className="flex items-center gap-2 border-b border-fd-border px-3 py-2">
              <span className="size-2 rounded-full bg-red-500" />
              <span className="text-fd-muted-foreground">shapes.clot:12</span>
              <div className="ml-auto flex items-center gap-1 rounded-md bg-fd-background p-0.5">
                {(['es', 'en'] as const).map((l) => (
                  <button
                    key={l}
                    type="button"
                    onClick={() => setLang(l)}
                    className={`rounded px-2 py-0.5 text-[11px] transition-colors ${
                      lang === l
                        ? 'bg-fd-primary text-white dark:text-fd-primary-foreground'
                        : 'text-fd-muted-foreground hover:text-fd-foreground'
                    }`}
                  >
                    {l}
                  </button>
                ))}
              </div>
            </div>
            <div key={lang} className="clot-fade-up space-y-1 p-3">
              <div>
                <span className="text-red-500">error:</span>
                <span className="text-fd-foreground">{d.msg}</span>
              </div>
              <div className={cm}>{d.help}</div>
            </div>
          </div>
        </div>
      </SpotlightCard>

      <SpotlightCard className="sm:col-span-2 lg:col-span-3">
        <div className="flex h-full flex-col p-6">
          <CardHead icon={WandSparklesIcon} title="The ergonomics you expect">
            Modern niceties are built in, not bolted on.
          </CardHead>
          <div className="mt-auto flex flex-wrap gap-2 pt-2">
            {ERGONOMICS.map((e) => (
              <span
                key={e}
                className="rounded-full border border-fd-border bg-fd-muted/40 px-3 py-1 font-mono text-xs text-fd-muted-foreground transition-colors hover:border-fd-primary/40 hover:text-fd-foreground"
              >
                {e}
              </span>
            ))}
          </div>
        </div>
      </SpotlightCard>
    </div>
  );
}

const ERGONOMICS = [
  'string interpolation',
  'try / catch / finally',
  'defer',
  'collection builtins',
  'first-class functions',
  'module imports',
  'get / set',
  'range()',
];
