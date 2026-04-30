import type { BaseLayoutProps } from 'fumadocs-ui/layouts/shared';
import { Logo } from '../components/logo';

export const gitConfig = {
  user: 'jclot',
  repo: 'ClotLang',
  branch: 'master',
};

export function baseOptions(): BaseLayoutProps {
  return {
    nav: {
      title: <Logo />,
    },
    githubUrl: `https://github.com/${gitConfig.user}/${gitConfig.repo}`,
    links: [
      {
        type: 'menu',
        text: 'Documentation',
        items: [
          {
            text: 'Getting Started',
            description: 'Learn the basics of ClotLang',
            url: '/docs',
          },
          {
            text: 'API Reference',
            description: 'Explore all available APIs',
            url: '/docs/api',
          },
        ],
      },
      {
        text: 'Blog',
        url: '/blog',
        active: 'nested-url',
      },
    ],
  };
}