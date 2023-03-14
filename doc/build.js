#!/usr/bin/env node

console.log('hello world');

// const fs            = require('fs');
// const esbuild       = require('esbuild');
// const webpackConfig = require('./webpack.config');

// const buildList = [];
// const styles    = [];

// // Re-use webpack config for entrypoints
// for(let [name, path] of Object.entries(webpackConfig.entry)) {

//   // Fix node_modules reference
//   if (path.substr(0, 1) !== '.') path = `../node_modules/${path}`;

//   // Fix missing extension
//   if (!~['js','ts'].indexOf(path.split('.').pop())) path = `${path}.js`;

//   // Run the actual build
//   let cfg;
//   const res = esbuild.buildSync(cfg = {
//     format: 'cjs',
//     target: 'es6',
//     bundle: true,
//     outfile: __dirname + `/dist/${name}.bundle.js`,
//     entryPoints: [path],
//     loader: {
//       '.ttf': 'dataurl',
//     },
//   });

//   console.log({cfg, res});

//   // Record file to load
//   buildList.push(`${name}.bundle.js`);
//   try {
//     fs.statSync(__dirname + `/dist/${name}.bundle.css`);
//     styles.push(`${name}.bundle.css`);
//   } catch(e) {
//     // Intentionally empty
//   }
// }

// fs.writeFileSync(__dirname + `/dist/index.html`, `
// <!DOCTYPE html>
// <html>
//   <head>
//     <meta charset="utf-8">
//     <title>${webpackConfig.plugins[0].title}x</title>}
//     <meta name="viewport" content="width=device-width, initial-scale=1">
//     ${styles.map(name => `<link rel="stylesheet" href="${name}"/>`).join('')}
//     ${buildList.map(name => `<script defer src="${name}"></script>`).join('')}
//   </head>
//   <body>
//   </body>
// </html>
// `);

