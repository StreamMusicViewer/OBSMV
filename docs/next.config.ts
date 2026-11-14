import nextra from 'nextra'

const isProd = process.env.NODE_ENV === 'production'

const withNextra = nextra({
  latex: true,
  defaultShowCopyCode: true,
  search: {
    codeblocks: false,
  },
})

export default withNextra({
  reactStrictMode: true,
  output: 'export',
  basePath: isProd ? '/OBSMV' : '',
  assetPrefix: isProd ? '/OBSMV' : '',
  trailingSlash: true,
  images: {
    unoptimized: true,
  },
  experimental: {
    optimizePackageImports: ['@/components/annotations', '@/components/icons'],
  },
  webpack(config) {
    const { test: _test, ...imageLoaderOptions } = config.module.rules.find(
      (rule: any) => rule.test?.test?.('.svg'),
    )
    config.module.rules.push({
      test: /\.svg$/,
      oneOf: [
        {
          resourceQuery: /svgr/,
          use: ['@svgr/webpack'],
        },
        imageLoaderOptions,
      ],
    })
    return config
  },
  turbopack: {
    resolveAlias: {
      'next-mdx-import-source-file': './mdx-components.tsx',
    },
    rules: {
      './components/annotations/**/*.svg': {
        loaders: ['@svgr/webpack'],
        as: '*.js',
      },
      './components/icons/**/*.svg': {
        loaders: ['@svgr/webpack'],
        as: '*.js',
      },
    },
  },
})
