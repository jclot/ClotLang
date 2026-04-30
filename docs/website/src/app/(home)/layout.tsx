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
  BookOpenIcon,
  CodeIcon,
  FileTextIcon,
  PlusIcon,
  RssIcon,
  TagIcon,
  ZapIcon,
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
    <div className={`mb-2.5 flex size-8 items-center justify-center rounded-lg ring-1 ${s.bg} ${s.ring}`}>
      <Icon className={`size-4 ${s.text}`} />
    </div>
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
            { text: 'Getting Started', url: `/docs/${v}`,            icon: <BookOpenIcon /> },
            { text: 'Language Guide',  url: `/docs/${v}/guide`,      icon: <FileTextIcon /> },
            { text: 'API Reference',   url: `/docs/${v}/api`,        icon: <CodeIcon />     },
            { text: 'Quick Start',     url: `/docs/${v}/quickstart`, icon: <ZapIcon />      },
            { text: 'Installation',    url: `/docs/${v}/install`,    icon: <PlusIcon />     },
          ],
        },
        {
          type: 'menu',
          on: 'menu',
          text: 'Blog',
          items: [
            { text: 'All Posts', url: '/blog',          icon: <RssIcon /> },
            { text: 'Releases',  url: '/blog/releases', icon: <TagIcon /> },
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

              <NavbarMenuContent>

                {/* Hero card — col 1, row-span-2 */}
                <NavbarMenuLink href={`/docs/${v}`} className="md:row-span-2 group">
                  <div className="-mx-3 -mt-3 mb-3 overflow-hidden rounded-t-lg h-36 relative bg-fd-primary/5 dark:bg-fd-primary/10">
                    <svg
                      className="absolute inset-0 w-full h-full opacity-30 dark:opacity-20"
                      xmlns="http://www.w3.org/2000/svg"
                    >
                      <defs>
                        <pattern id="dots-doc" x="0" y="0" width="20" height="20" patternUnits="userSpaceOnUse">
                          <circle cx="2" cy="2" r="1.5" className="fill-fd-primary" />
                        </pattern>
                      </defs>
                      <rect width="100%" height="100%" fill="url(#dots-doc)" />
                    </svg>
                    <div className="absolute inset-0 flex items-center justify-center">
                      <div className="flex size-14 items-center justify-center rounded-2xl bg-fd-primary/10 ring-1 ring-fd-primary/20 backdrop-blur-sm group-hover:bg-fd-primary/15 transition-colors">
                        <BookOpenIcon className="size-7 text-fd-primary" />
                      </div>
                    </div>
                  </div>
                  <p className="font-semibold text-sm text-fd-foreground">Getting Started</p>
                  <p className="text-fd-muted-foreground text-xs leading-relaxed mt-1">
                    Build your first ClotLang project from scratch.
                  </p>
                </NavbarMenuLink>

                {/* col 2, row 1 */}
                <NavbarMenuLink href={`/docs/${v}/guide`} className="lg:col-start-2 group">
                  <NavIcon icon={FileTextIcon} color="violet" />
                  <p className="font-medium text-sm text-fd-foreground">Language Guide</p>
                  <p className="text-fd-muted-foreground text-xs leading-relaxed mt-0.5">
                    Syntax, types and core concepts.
                  </p>
                </NavbarMenuLink>

                {/* col 2, row 2 */}
                <NavbarMenuLink href={`/docs/${v}/quickstart`} className="lg:col-start-2 group">
                  <NavIcon icon={ZapIcon} color="amber" />
                  <p className="font-medium text-sm text-fd-foreground">Quick Start</p>
                  <p className="text-fd-muted-foreground text-xs leading-relaxed mt-0.5">
                    Up and running in under 5 minutes.
                  </p>
                </NavbarMenuLink>

                {/* col 3, row 1 */}
                <NavbarMenuLink href={`/docs/${v}/api`} className="lg:col-start-3 lg:row-start-1 group">
                  <NavIcon icon={CodeIcon} color="sky" />
                  <p className="font-medium text-sm text-fd-foreground">API Reference</p>
                  <p className="text-fd-muted-foreground text-xs leading-relaxed mt-0.5">
                    Built-in functions and types.
                  </p>
                </NavbarMenuLink>

                {/* col 3, row 2 */}
                <NavbarMenuLink href={`/docs/${v}/install`} className="lg:col-start-3 lg:row-start-2 group">
                  <NavIcon icon={PlusIcon} color="emerald" />
                  <p className="font-medium text-sm text-fd-foreground">Installation</p>
                  <p className="text-fd-muted-foreground text-xs leading-relaxed mt-0.5">
                    Add ClotLang to any project.
                  </p>
                </NavbarMenuLink>

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

              <NavbarMenuContent>

                {/* Hero card — col 1, row-span-2 */}
                <NavbarMenuLink href="/blog" className="md:row-span-2 group">
                  <div className="-mx-3 -mt-3 mb-3 overflow-hidden rounded-t-lg h-28 relative bg-rose-500/5 dark:bg-rose-400/10">
                    <svg
                      className="absolute inset-0 w-full h-full opacity-30 dark:opacity-20"
                      xmlns="http://www.w3.org/2000/svg"
                    >
                      <defs>
                        <pattern id="dots-blog" x="0" y="0" width="20" height="20" patternUnits="userSpaceOnUse">
                          <circle cx="2" cy="2" r="1.5" className="fill-rose-500" />
                        </pattern>
                      </defs>
                      <rect width="100%" height="100%" fill="url(#dots-blog)" />
                    </svg>
                    <div className="absolute inset-0 flex items-center justify-center">
                      <div className="flex size-12 items-center justify-center rounded-2xl bg-rose-500/10 ring-1 ring-rose-500/20 backdrop-blur-sm group-hover:bg-rose-500/15 transition-colors">
                        <RssIcon className="size-6 text-rose-500 dark:text-rose-400" />
                      </div>
                    </div>
                  </div>
                  <p className="font-semibold text-sm text-fd-foreground">All Posts</p>
                  <p className="text-fd-muted-foreground text-xs leading-relaxed mt-1">
                    News, tutorials and updates about ClotLang.
                  </p>
                </NavbarMenuLink>

                {/* col 2, row 1 */}
                {/* <NavbarMenuLink href="/blog/releases" className="lg:col-start-2 group">
                  <div className="flex items-center gap-2 mb-2.5">
                    <NavIcon icon={TagIcon} color="rose" />
                    <span className="rounded-full bg-fd-primary/10 px-2 py-0.5 text-[10px] font-medium text-fd-primary -mt-1">
                      New
                    </span>
                  </div>
                  <p className="font-medium text-sm text-fd-foreground">Releases</p>
                  <p className="text-fd-muted-foreground text-xs leading-relaxed mt-0.5">
                    Version announcements and changelogs.
                  </p>
                </NavbarMenuLink> */}

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