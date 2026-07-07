'use client';

import { useState } from 'react';
import { ZapIcon, RocketIcon, ShieldCheckIcon } from 'lucide-react';

type Mode = {
  id: string;
  label: string;
  icon: React.ElementType;
  title: string;
  description: string;
  command: string;
  output: { text: string; tone?: 'ok' | 'muted' | 'accent' }[];
};

const MODES: Mode[] = [
  {
    id: 'interpret',
    label: 'Interpret',
    icon: ZapIcon,
    title: 'Run instantly',
    description:
      'The interpreter executes your file directly. No build step, no boilerplate — the fastest possible feedback loop for learning and scripting.',
    command: 'clot app.clot',
    output: [
      { text: 'Hello from ClotLang!', tone: 'muted' },
      { text: 'done in 4ms', tone: 'accent' },
    ],
  },
  {
    id: 'compile',
    label: 'Compile',
    icon: RocketIcon,
    title: 'Ship a native binary',
    description:
      'The same source, lowered through LLVM to a standalone executable when you need real performance and a single artifact to distribute.',
    command: 'clot app.clot --mode compile --emit exe -o app',
    output: [
      { text: 'clot: lowering to LLVM IR…', tone: 'muted' },
      { text: 'clot: linking with lld…', tone: 'muted' },
      { text: '→ ./app  (native executable)', tone: 'ok' },
    ],
  },
  {
    id: 'analyze',
    label: 'Analyze',
    icon: ShieldCheckIcon,
    title: 'Catch errors early',
    description:
      'The static analyzer verifies your type annotations and structure before anything runs, so mistakes surface at your desk — not in production.',
    command: 'clot app.clot --mode analyze',
    output: [
      { text: 'clot: analyzing 1 module…', tone: 'muted' },
      { text: '✓ no issues found', tone: 'ok' },
    ],
  },
];

export function ModeTabs() {
  const [active, setActive] = useState(0);
  const mode = MODES[active];

  return (
    <div className="mx-auto max-w-3xl">
      {/* Tab bar */}
      <div className="mx-auto mb-8 flex w-fit items-center gap-1 rounded-full border border-fd-border bg-fd-card p-1">
        {MODES.map((m, i) => {
          const Icon = m.icon;
          const isActive = i === active;
          return (
            <button
              key={m.id}
              type="button"
              onClick={() => setActive(i)}
              className={`inline-flex items-center gap-2 rounded-full px-4 py-2 text-sm font-medium transition-colors ${
                isActive
                  ? 'bg-fd-primary text-white dark:text-fd-primary-foreground'
                  : 'text-fd-muted-foreground hover:text-fd-foreground'
              }`}
            >
              <Icon className="size-4" />
              {m.label}
            </button>
          );
        })}
      </div>

      {/* Panel */}
      <div
        key={mode.id}
        className="clot-fade-up grid grid-cols-1 items-center gap-8 md:grid-cols-2"
      >
        <div>
          <h3 className="text-2xl font-bold tracking-tight text-fd-foreground">
            {mode.title}
          </h3>
          <p className="mt-3 leading-relaxed text-fd-muted-foreground">
            {mode.description}
          </p>
        </div>

        <div className="overflow-hidden rounded-xl border border-fd-border bg-fd-card shadow-lg shadow-black/5">
          <div className="flex items-center gap-2 border-b border-fd-border bg-fd-muted/40 px-4 py-2.5">
            <span className="size-2.5 rounded-full bg-red-400/80" />
            <span className="size-2.5 rounded-full bg-yellow-400/80" />
            <span className="size-2.5 rounded-full bg-green-400/80" />
          </div>
          <div className="space-y-2 px-4 py-4 font-mono text-xs">
            <div className="overflow-x-auto whitespace-nowrap text-fd-foreground">
              <span className="select-none text-fd-primary">$</span>{' '}
              {mode.command}
            </div>
            <div className="space-y-1">
              {mode.output.map((o, i) => (
                <div
                  key={i}
                  className={
                    o.tone === 'ok'
                      ? 'text-emerald-600 dark:text-emerald-400'
                      : o.tone === 'accent'
                        ? 'text-fd-primary'
                        : 'text-fd-muted-foreground'
                  }
                >
                  {o.text}
                </div>
              ))}
            </div>
          </div>
        </div>
      </div>
    </div>
  );
}
