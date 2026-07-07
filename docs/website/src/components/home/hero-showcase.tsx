'use client';

import { useEffect, useState } from 'react';
import { TerminalIcon } from 'lucide-react';

const COMMAND = 'clot hello.clot';
const OUTPUT = [
  'Hello from ClotLang!',
  'Hi, world',
  'line 0',
  'line 1',
  'line 2',
];

type Phase = 'typing' | 'running' | 'output' | 'hold';

export function HeroShowcase() {
  const [typed, setTyped] = useState('');
  const [lines, setLines] = useState(0);
  const [phase, setPhase] = useState<Phase>('typing');

  useEffect(() => {
    let timer: ReturnType<typeof setTimeout>;

    if (phase === 'typing') {
      if (typed.length < COMMAND.length) {
        timer = setTimeout(
          () => setTyped(COMMAND.slice(0, typed.length + 1)),
          65,
        );
      } else {
        timer = setTimeout(() => setPhase('running'), 450);
      }
    } else if (phase === 'running') {
      timer = setTimeout(() => setPhase('output'), 350);
    } else if (phase === 'output') {
      if (lines < OUTPUT.length) {
        timer = setTimeout(() => setLines((n) => n + 1), 240);
      } else {
        timer = setTimeout(() => setPhase('hold'), 3200);
      }
    } else {
      timer = setTimeout(() => {
        setTyped('');
        setLines(0);
        setPhase('typing');
      }, 400);
    }

    return () => clearTimeout(timer);
  }, [typed, lines, phase]);

  return (
    <div className="clot-float w-full max-w-lg">
      <div className="overflow-hidden rounded-2xl border border-fd-border bg-fd-card shadow-2xl shadow-black/20 ring-1 ring-black/5 dark:ring-white/5">
        {/* Title bar */}
        <div className="flex items-center gap-2 border-b border-fd-border bg-fd-muted/40 px-4 py-2.5">
          <span className="size-3 rounded-full bg-red-400/80" />
          <span className="size-3 rounded-full bg-yellow-400/80" />
          <span className="size-3 rounded-full bg-green-400/80" />
          <span className="ml-2 text-xs font-medium text-fd-muted-foreground">
            hello.clot
          </span>
        </div>

        {/* Source */}
        <pre className="overflow-x-auto px-5 py-4 text-[13px] leading-relaxed">
          <code className="font-mono">
            <Line n={1}>
              <T.fn>println</T.fn>
              <T.p>(</T.p>
              <T.str>&quot;Hello from ClotLang!&quot;</T.str>
              <T.p>);</T.p>
            </Line>
            <Line n={2}> </Line>
            <Line n={3}>
              <T.v>name</T.v>
              <T.p> = </T.p>
              <T.str>&quot;world&quot;</T.str>
              <T.p>;</T.p>
            </Line>
            <Line n={4}>
              <T.fn>println</T.fn>
              <T.p>(</T.p>
              <T.str>&quot;Hi, &#123;name&#125;&quot;</T.str>
              <T.p>);</T.p>
            </Line>
            <Line n={5}> </Line>
            <Line n={6}>
              <T.kw>for</T.kw>
              <T.v> i </T.v>
              <T.kw>in</T.kw>
              <T.fn> range</T.fn>
              <T.p>(</T.p>
              <T.num>3</T.num>
              <T.p>):</T.p>
            </Line>
            <Line n={7}>
              {'    '}
              <T.fn>println</T.fn>
              <T.p>(</T.p>
              <T.str>&quot;line &#123;i&#125;&quot;</T.str>
              <T.p>);</T.p>
            </Line>
            <Line n={8}>
              <T.kw>endfor</T.kw>
            </Line>
          </code>
        </pre>

        {/* Terminal */}
        <div className="border-t border-fd-border bg-fd-muted/30 px-5 py-3.5 font-mono text-xs">
          <div className="mb-2 flex items-center gap-1.5 text-fd-muted-foreground">
            <TerminalIcon className="size-3.5" />
            Terminal
          </div>
          <div className="text-fd-foreground">
            <span className="text-fd-primary">$</span> {typed}
            {phase === 'typing' && (
              <span className="clot-caret ml-0.5 inline-block h-3.5 w-1.5 translate-y-0.5 bg-fd-primary" />
            )}
          </div>
          <div className="mt-1 space-y-0.5 text-fd-muted-foreground">
            {OUTPUT.slice(0, lines).map((l, i) => (
              <div key={i} className="clot-fade-up">
                {l}
              </div>
            ))}
          </div>
        </div>
      </div>
    </div>
  );
}

function Line({ n, children }: { n: number; children: React.ReactNode }) {
  return (
    <span className="grid grid-cols-[1.5rem_1fr] gap-3">
      <span className="select-none text-right text-fd-muted-foreground/40">
        {n}
      </span>
      <span>{children}</span>
    </span>
  );
}

/* token colors */
const T = {
  fn: (p: { children: React.ReactNode }) => (
    <span className="text-fd-primary">{p.children}</span>
  ),
  str: (p: { children: React.ReactNode }) => (
    <span className="text-emerald-600 dark:text-emerald-400">{p.children}</span>
  ),
  kw: (p: { children: React.ReactNode }) => (
    <span className="text-violet-600 dark:text-violet-400">{p.children}</span>
  ),
  num: (p: { children: React.ReactNode }) => (
    <span className="text-amber-600 dark:text-amber-400">{p.children}</span>
  ),
  v: (p: { children: React.ReactNode }) => (
    <span className="text-fd-foreground">{p.children}</span>
  ),
  p: (p: { children: React.ReactNode }) => (
    <span className="text-fd-muted-foreground">{p.children}</span>
  ),
};
