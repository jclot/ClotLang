import Link from 'next/link';
import { ArrowRightIcon } from 'lucide-react';
import type { Metadata } from 'next';
import { blog } from '@/lib/source';

export const metadata: Metadata = {
  title: 'Blog',
  description: 'News, tutorials, and updates about ClotLang.',
};

function formatDate(date: Date) {
  return new Date(date).toLocaleDateString('en-US', {
    year: 'numeric',
    month: 'short',
    day: 'numeric',
  });
}

function Avatar({ name }: { name: string }) {
  const initials = name
    .split(' ')
    .map((p) => p[0])
    .slice(0, 2)
    .join('')
    .toUpperCase();
  return (
    <span className="flex size-6 shrink-0 items-center justify-center rounded-full bg-fd-primary/15 text-[10px] font-semibold text-fd-primary ring-1 ring-fd-primary/20">
      {initials}
    </span>
  );
}

export default function BlogIndex() {
  const posts = [...blog.getPages()].sort(
    (a, b) =>
      new Date(b.data.date).getTime() - new Date(a.data.date).getTime(),
  );

  const [featured, ...rest] = posts;

  return (
    <main className="relative flex flex-1 flex-col">
      {/* Decorative top glow */}
      <div
        aria-hidden
        className="pointer-events-none absolute inset-x-0 top-0 -z-10 h-80"
        style={{
          background:
            'radial-gradient(ellipse 60% 100% at 50% 0%, color-mix(in oklch, var(--color-fd-primary) 16%, transparent), transparent 70%)',
        }}
      />

      <div className="mx-auto w-full max-w-5xl px-4 py-16 sm:py-20">
        {/* Header */}
        <header className="mb-12 text-center">
          <p className="mb-2 text-sm font-medium text-fd-primary">The ClotLang Blog</p>
          <h1 className="text-4xl font-bold tracking-tight sm:text-5xl">
            News &amp; writing
          </h1>
          <p className="mx-auto mt-4 max-w-lg text-fd-muted-foreground">
            Release announcements, language design notes, and tutorials from the
            people building ClotLang.
          </p>
        </header>

        {/* Featured post */}
        {featured && (
          <Link
            href={featured.url}
            className="group relative mb-8 block overflow-hidden rounded-2xl border border-fd-border bg-fd-card p-6 transition-colors hover:border-fd-primary/40 sm:p-8"
          >
            <div
              aria-hidden
              className="pointer-events-none absolute inset-0 -z-10 opacity-60"
              style={{
                background:
                  'radial-gradient(ellipse 70% 120% at 100% 0%, color-mix(in oklch, var(--color-fd-primary) 14%, transparent), transparent 60%)',
              }}
            />
            <div className="flex flex-wrap items-center gap-3 text-xs">
              {featured.data.tag && (
                <span className="rounded-full bg-fd-primary/10 px-2.5 py-1 font-medium text-fd-primary ring-1 ring-fd-primary/20">
                  {featured.data.tag}
                </span>
              )}
              <span className="text-fd-muted-foreground">
                {formatDate(featured.data.date)}
              </span>
              <span className="rounded-full border border-fd-border px-2.5 py-1 font-medium text-fd-muted-foreground">
                Latest
              </span>
            </div>
            <h2 className="mt-4 text-2xl font-bold tracking-tight text-fd-foreground sm:text-3xl">
              {featured.data.title}
            </h2>
            <p className="mt-3 max-w-2xl text-fd-muted-foreground">
              {featured.data.description}
            </p>
            <div className="mt-6 flex items-center justify-between">
              <div className="flex items-center gap-2 text-sm text-fd-muted-foreground">
                <Avatar name={featured.data.author} />
                {featured.data.author}
              </div>
              <span className="inline-flex items-center gap-1 text-sm font-medium text-fd-primary">
                Read post
                <ArrowRightIcon className="size-4 transition-transform group-hover:translate-x-0.5" />
              </span>
            </div>
          </Link>
        )}

        {/* Grid of the rest */}
        <div className="grid grid-cols-1 gap-5 sm:grid-cols-2 lg:grid-cols-3">
          {rest.map((post) => (
            <Link
              key={post.url}
              href={post.url}
              className="group flex flex-col rounded-xl border border-fd-border bg-fd-card p-5 transition-colors hover:border-fd-primary/40"
            >
              <div className="mb-3 flex items-center gap-2 text-xs">
                {post.data.tag && (
                  <span className="rounded-full bg-fd-primary/10 px-2 py-0.5 font-medium text-fd-primary ring-1 ring-fd-primary/20">
                    {post.data.tag}
                  </span>
                )}
                <span className="text-fd-muted-foreground">
                  {formatDate(post.data.date)}
                </span>
              </div>
              <h3 className="font-semibold text-fd-foreground group-hover:text-fd-primary">
                {post.data.title}
              </h3>
              <p className="mt-2 flex-1 text-sm leading-relaxed text-fd-muted-foreground">
                {post.data.description}
              </p>
              <div className="mt-4 flex items-center gap-2 text-xs text-fd-muted-foreground">
                <Avatar name={post.data.author} />
                {post.data.author}
              </div>
            </Link>
          ))}
        </div>
      </div>
    </main>
  );
}
