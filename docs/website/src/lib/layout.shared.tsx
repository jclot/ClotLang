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
            text: 'Overview',
            description: 'Start here — what ClotLang is and how to read the docs',
            url: '/docs',
          },
          {
            text: 'Setup Guide',
            description: 'Install ClotLang or build it from source',
            url: '/docs',
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