import { HomeLayout } from 'fumadocs-ui/layouts/home';
import { baseOptions } from '@/lib/layout.shared';
import {
  NavbarMenu,
  NavbarMenuContent,
  NavbarMenuLink,
  NavbarMenuTrigger,
} from 'fumadocs-ui/layouts/home/navbar';
import Link from 'fumadocs-core/link';
import {
  ArrowRightIcon,
  BookOpenIcon,
  CompassIcon,
  DownloadIcon,
  PlayIcon,
  RssIcon,
  ScaleIcon,
  SparklesIcon,
  TagIcon,
} from 'lucide-react';
import type { ReactNode } from 'react';
import { getLatestDocsVersion } from '@/lib/docs-versions';

function NavIcon({
  icon: Icon,
  color,
}: {
  icon: React.ElementType;
  color: 'violet' | 'sky' | 'emerald' | 'rose' | 'amber';
}) {
  const styles: Record<string, { bg: string; text: string; ring: string }> = {
    violet:  { bg: 'bg-violet-500/10 dark:bg-violet-400/10',   text: 'text-violet-600 dark:text-violet-400',   ring: 'ring-violet-500/20'  },
    sky:     { bg: 'bg-sky-500/10 dark:bg-sky-400/10',         text: 'text-sky-600 dark:text-sky-400',         ring: 'ring-sky-500/20'     },
    emerald: { bg: 'bg-emerald-500/10 dark:bg-emerald-400/10', text: 'text-emerald-600 dark:text-emerald-400', ring: 'ring-emerald-500/20' },
    rose:    { bg: 'bg-rose-500/10 dark:bg-rose-400/10',       text: 'text-rose-600 dark:text-rose-400',       ring: 'ring-rose-500/20'    },
    amber:   { bg: 'bg-amber-500/10 dark:bg-amber-400/10',     text: 'text-amber-600 dark:text-amber-400',     ring: 'ring-amber-500/20'   },
  };
  const s = styles[color];
  return (
    <div className={`mb-3 flex size-9 items-center justify-center rounded-lg ring-1 transition-transform group-hover:scale-105 ${s.bg} ${s.ring}`}>
      <Icon className={`size-4.5 ${s.text}`} />
    </div>
  );
}

/** A compact link row used inside the dropdown, styled like fumadocs' menus. */
function MenuRow({
  href,
  icon: Icon,
  color,
  title,
  description,
  className,
}: {
  href: string;
  icon: React.ElementType;
  color: 'violet' | 'sky' | 'emerald' | 'rose' | 'amber';
  title: string;
  description: string;
  className?: string;
}) {
  return (
    <NavbarMenuLink href={href} className={`group ${className ?? ''}`}>
      <NavIcon icon={Icon} color={color} />
      <p className="flex items-center gap-1 text-sm font-medium text-fd-foreground">
        {title}
        <ArrowRightIcon className="size-3 -translate-x-1 opacity-0 transition-all group-hover:translate-x-0 group-hover:opacity-100" />
      </p>
      <p className="mt-0.5 text-xs leading-relaxed text-fd-muted-foreground">
        {description}
      </p>
    </NavbarMenuLink>
  );
}

export default function Layout({ children }: { children: ReactNode }) {
  const v = getLatestDocsVersion();

  return (
    <HomeLayout
      {...baseOptions()}
      links={[
        // ── Móvil ───────────────────────────────────────────────────────
        {
          type: 'menu',
          on: 'menu',
          text: 'Documentation',
          items: [
            { text: 'Overview',           url: `/docs/${v}`,                      icon: <BookOpenIcon /> },
            { text: 'Setup Guide',        url: `/docs/${v}/installation`,         icon: <DownloadIcon /> },
            { text: 'First Run',          url: `/docs/${v}/first_run`,            icon: <PlayIcon />     },
            { text: 'Why ClotLang',       url: `/docs/${v}/why_clotlang_exists`,  icon: <SparklesIcon /> },
            { text: 'Design Trade-offs',  url: `/docs/${v}/design_trade_offs`,    icon: <ScaleIcon />    },
          ],
        },
        {
          type: 'menu',
          on: 'menu',
          text: 'Blog',
          items: [
            { text: 'All Posts', url: '/blog',                                icon: <RssIcon /> },
            { text: 'Releases',  url: '/blog/introducing-clotlang-0-3-0',     icon: <TagIcon /> },
          ],
        },

        // ── Desktop: Documentation ──────────────────────────────────────
        {
          type: 'custom',
          on: 'nav',
          children: (
            <NavbarMenu>
              <NavbarMenuTrigger>
                <Link href={`/docs/${v}`}>Documentation</Link>
              </NavbarMenuTrigger>

              <NavbarMenuContent className="grid-cols-2 lg:grid-cols-3">

                {/* Hero card — spans the first column across both rows */}
                <NavbarMenuLink href={`/docs/${v}`} className="group md:row-span-2 md:flex md:flex-col">
                  <div className="-mx-3 -mt-3 mb-3 h-40 overflow-hidden rounded-t-lg relative bg-gradient-to-br from-fd-primary/15 via-fd-primary/5 to-transparent">
                    <svg
                      className="absolute inset-0 h-full w-full opacity-40 dark:opacity-25"
                      xmlns="http://www.w3.org/2000/svg"
                    >
                      <defs>
                        <pattern id="grid-doc" x="0" y="0" width="22" height="22" patternUnits="userSpaceOnUse">
                          <path d="M22 0H0V22" fill="none" className="stroke-fd-primary" strokeWidth="0.5" />
                        </pattern>
                      </defs>
                      <rect width="100%" height="100%" fill="url(#grid-doc)" />
                    </svg>
                    <div className="absolute inset-0 flex items-center justify-center">
                      <div className="flex size-16 items-center justify-center rounded-2xl bg-fd-primary/10 ring-1 ring-fd-primary/20 backdrop-blur-sm transition-all group-hover:scale-105 group-hover:bg-fd-primary/15">
                        <CompassIcon className="size-8 text-fd-primary" />
                      </div>
                    </div>
                  </div>
                  <p className="flex items-center gap-1 text-sm font-semibold text-fd-foreground">
                    Overview
                    <ArrowRightIcon className="size-3 -translate-x-1 opacity-0 transition-all group-hover:translate-x-0 group-hover:opacity-100" />
                  </p>
                  <p className="mt-1 text-xs leading-relaxed text-fd-muted-foreground">
                    Start here — what ClotLang is, how to read the docs, and where
                    to go next.
                  </p>
                </NavbarMenuLink>

                {/* col 2 */}
                <MenuRow
                  href={`/docs/${v}/installation`}
                  icon={DownloadIcon}
                  color="emerald"
                  title="Setup Guide"
                  description="Install on Linux, macOS, or Windows, or build from source."
                  className="lg:col-start-2"
                />
                <MenuRow
                  href={`/docs/${v}/first_run`}
                  icon={PlayIcon}
                  color="amber"
                  title="First Run"
                  description="Run your first .clot file and confirm the toolchain works."
                  className="lg:col-start-2"
                />

                {/* col 3 */}
                <MenuRow
                  href={`/docs/${v}/why_clotlang_exists`}
                  icon={SparklesIcon}
                  color="violet"
                  title="Why ClotLang"
                  description="The philosophy and goals behind the language."
                  className="lg:col-start-3 lg:row-start-1"
                />
                <MenuRow
                  href={`/docs/${v}/design_trade_offs`}
                  icon={ScaleIcon}
                  color="sky"
                  title="Design Trade-offs"
                  description="The deliberate choices that shape how ClotLang feels."
                  className="lg:col-start-3 lg:row-start-2"
                />

              </NavbarMenuContent>
            </NavbarMenu>
          ),
        },

        // ── Desktop: Blog ───────────────────────────────────────────────
        {
          type: 'custom',
          on: 'nav',
          children: (
            <NavbarMenu>
              <NavbarMenuTrigger>
                <Link href="/blog">Blog</Link>
              </NavbarMenuTrigger>

              <NavbarMenuContent className="grid-cols-2">

                {/* Hero card */}
                <NavbarMenuLink href="/blog" className="group md:row-span-2 md:flex md:flex-col">
                  <div className="-mx-3 -mt-3 mb-3 h-32 overflow-hidden rounded-t-lg relative bg-gradient-to-br from-rose-500/15 via-rose-500/5 to-transparent">
                    <svg
                      className="absolute inset-0 h-full w-full opacity-40 dark:opacity-25"
                      xmlns="http://www.w3.org/2000/svg"
                    >
                      <defs>
                        <pattern id="grid-blog" x="0" y="0" width="22" height="22" patternUnits="userSpaceOnUse">
                          <path d="M22 0H0V22" fill="none" className="stroke-rose-500" strokeWidth="0.5" />
                        </pattern>
                      </defs>
                      <rect width="100%" height="100%" fill="url(#grid-blog)" />
                    </svg>
                    <div className="absolute inset-0 flex items-center justify-center">
                      <div className="flex size-14 items-center justify-center rounded-2xl bg-rose-500/10 ring-1 ring-rose-500/20 backdrop-blur-sm transition-all group-hover:scale-105 group-hover:bg-rose-500/15">
                        <RssIcon className="size-7 text-rose-500 dark:text-rose-400" />
                      </div>
                    </div>
                  </div>
                  <p className="flex items-center gap-1 text-sm font-semibold text-fd-foreground">
                    All Posts
                    <ArrowRightIcon className="size-3 -translate-x-1 opacity-0 transition-all group-hover:translate-x-0 group-hover:opacity-100" />
                  </p>
                  <p className="mt-1 text-xs leading-relaxed text-fd-muted-foreground">
                    News, tutorials, and updates about ClotLang.
                  </p>
                </NavbarMenuLink>

                {/* col 2 */}
                <MenuRow
                  href="/blog/introducing-clotlang-0-3-0"
                  icon={TagIcon}
                  color="rose"
                  title="Releases"
                  description="Version announcements and changelogs."
                  className="lg:col-start-2"
                />
                <MenuRow
                  href={`/docs/${v}`}
                  icon={BookOpenIcon}
                  color="violet"
                  title="Documentation"
                  description="Jump straight into the reference guides."
                  className="lg:col-start-2"
                />

              </NavbarMenuContent>
            </NavbarMenu>
          ),
        },
      ]}
    >
      {children}
    </HomeLayout>
  );
}
