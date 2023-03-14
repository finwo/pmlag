#!/usr/bin/env node

const fs            = require('fs');
const esbuild       = require('esbuild');
const esbuildSvelte = require('esbuild-svelte');
const preprocess    = require('svelte-preprocess');
const pkg           = require('./package.json');

const pageroot = process.env.PAGE_ROOT || '/';

// Simple for now
const entryPoints = {
  'app': 'src/app.ts'
};

const config = {
  format: 'cjs',
  target: ['chrome108','firefox107'],
  mainFields: ['svelte', 'browser', 'module', 'main'],
  bundle: true,
  outdir: __dirname + '/dist',
  entryPoints: Object.values(entryPoints),
  minify: false,
  plugins: [
    esbuildSvelte({
      preprocess: preprocess(),
    }),
  ],
  // loader: {
  //   '.ttf': 'dataurl',
  // },
}

const buildList = [];
const styles    = [];

esbuild
  .build(config)
  .then(() => {

    for(const name of Object.keys(entryPoints)) {
      buildList.push(`${name}.js`);
      try {
        fs.statSync(config.outdir + `/${name}.css`);
        styles.push(`${name}.css`);
      } catch(_e) {
        // Intentionally empty
      }
    }

    fs.writeFileSync(config.outdir + '/index.html', `<!DOCTYPE html>
<html>
  <head>
    <meta charset="utf-8">
    <meta name="viewport" content="width=device-width,initial-scale=1">
    <style>
      :root {
        font-family: sans-serif;
      }
      * {
        box-sizing: border-box;
        font-family: inherit;
      }
    </style>
    ${styles.map(name => `<link rel="stylesheet" href="${pageroot}${name}"/>`).join('\n    ')}
  </head>
  <body>
    ${buildList.map(name => `<script defer src="${pageroot}${name}"></script>`).join('\n    ')}
  </body>
</html>
`);

    // Copy index.html to 404.html
    fs.writeFileSync(config.outdir + '/404.html', fs.readFileSync(config.outdir + '/index.html'));

  });
