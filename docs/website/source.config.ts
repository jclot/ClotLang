import {
  defineConfig,
  defineDocs,
  defineCollections,
  frontmatterSchema,
} from 'fumadocs-mdx/config';
import { metaSchema, pageSchema } from 'fumadocs-core/source/schema';
import { z } from 'zod';
import { readFileSync } from 'fs';
import { join } from 'path';

const clotGrammar = JSON.parse(
  readFileSync(join(process.cwd(), './lib/clot.tmLanguage.json'), 'utf-8')
);

const clotLanguage = {
  ...clotGrammar,
  name: 'clot',
  aliases: ['clotlang'],
} as any;

export const docs = defineDocs({
  dir: 'content/docs',
  docs: {
    schema: pageSchema,
    postprocess: {
      includeProcessedMarkdown: true,
    },
  },
  meta: {
    schema: metaSchema,
  },
});

export const blog = defineCollections({
  type: 'doc',
  dir: 'content/blog',
  schema: frontmatterSchema.extend({
    author: z.string(),
    date: z.coerce.date(),
    tag: z.string().optional(),
  }),
});

export default defineConfig({
  mdxOptions: {
    rehypeCodeOptions: {
      themes: {
        light: 'github-light',
        dark: 'github-dark',
      },
      langs: [clotLanguage], 
    },
  },
});