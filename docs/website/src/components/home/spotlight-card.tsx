'use client';

import { useRef, type ReactNode } from 'react';

export function SpotlightCard({
  children,
  className = '',
}: {
  children: ReactNode;
  className?: string;
}) {
  const ref = useRef<HTMLDivElement>(null);

  function onMouseMove(e: React.MouseEvent<HTMLDivElement>) {
    const el = ref.current;
    if (!el) return;
    const rect = el.getBoundingClientRect();
    el.style.setProperty('--x', `${e.clientX - rect.left}px`);
    el.style.setProperty('--y', `${e.clientY - rect.top}px`);
  }

  return (
    <div
      ref={ref}
      onMouseMove={onMouseMove}
      className={`group relative overflow-hidden rounded-xl border border-fd-border bg-fd-card transition-colors duration-300 hover:border-fd-primary/40 ${className}`}
    >
      {/* cursor-following glow */}
      <div
        aria-hidden
        className="pointer-events-none absolute inset-0 opacity-0 transition-opacity duration-300 group-hover:opacity-100"
        style={{
          background:
            'radial-gradient(300px circle at var(--x, 50%) var(--y, 50%), color-mix(in oklch, var(--color-fd-primary) 14%, transparent), transparent 70%)',
        }}
      />
      <div className="relative">{children}</div>
    </div>
  );
}
