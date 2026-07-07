import Link from 'next/link';
import { notFound } from 'next/navigation';
import { ArrowLeftIcon } from 'lucide-react';
import type { Metadata } from 'next';
import { blog } from '@/lib/source';
import { getMDXComponents } from '@/components/mdx';

function formatDate(date: Date) {
  return new Date(date).toLocaleDateString('en-US', {
    year: 'numeric',
    month: 'long',
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
    <span className="flex size-9 shrink-0 items-center justify-center rounded-full bg-fd-primary/15 text-xs font-semibold text-fd-primary ring-1 ring-fd-primary/20">
      {initials}
    </span>
  );
}

export default async function BlogPost(props: {
  params: Promise<{ slug: string }>;
}) {
  const { slug } = await props.params;
  const page = blog.getPage([slug]);
  if (!page) notFound();

  const MDX = page.data.body;

  return (
    <main className="relative flex flex-1 flex-col">
      <div
        aria-hidden
        className="pointer-events-none absolute inset-x-0 top-0 -z-10 h-72"
        style={{
          background:
            'radial-gradient(ellipse 60% 100% at 50% 0%, color-mix(in oklch, var(--color-fd-primary) 14%, transparent), transparent 70%)',
        }}
      />

      <article className="mx-auto w-full max-w-3xl px-4 py-14 sm:py-20">
        <Link
          href="/blog"
          className="mb-8 inline-flex items-center gap-1.5 text-sm text-fd-muted-foreground transition-colors hover:text-fd-foreground"
        >
          <ArrowLeftIcon className="size-4" />
          All posts
        </Link>

        <header className="mb-10 border-b border-fd-border pb-8">
          <div className="mb-4 flex flex-wrap items-center gap-3 text-xs">
            {page.data.tag && (
              <span className="rounded-full bg-fd-primary/10 px-2.5 py-1 font-medium text-fd-primary ring-1 ring-fd-primary/20">
                {page.data.tag}
              </span>
            )}
            <span className="text-fd-muted-foreground">
              {formatDate(page.data.date)}
            </span>
          </div>
          <h1 className="text-3xl font-bold tracking-tight sm:text-4xl">
            {page.data.title}
          </h1>
          <p className="mt-4 text-lg text-fd-muted-foreground">
            {page.data.description}
          </p>
          <div className="mt-6 flex items-center gap-3">
            <Avatar name={page.data.author} />
            <div className="text-sm">
              <p className="font-medium text-fd-foreground">
                {page.data.author}
              </p>
              <p className="text-fd-muted-foreground">Author</p>
            </div>
          </div>
        </header>

        <div className="prose">
          <MDX components={getMDXComponents()} />
        </div>
      </article>
    </main>
  );
}

export function generateStaticParams() {
  return blog.getPages().map((page) => ({ slug: page.slugs[0] }));
}

export async function generateMetadata(props: {
  params: Promise<{ slug: string }>;
}): Promise<Metadata> {
  const { slug } = await props.params;
  const page = blog.getPage([slug]);
  if (!page) notFound();

  return {
    title: page.data.title,
    description: page.data.description,
  };
}
