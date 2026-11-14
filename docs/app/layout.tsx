import type { Metadata } from 'next'
import { Head } from 'nextra/components'
import { getPageMap } from 'nextra/page-map'
import { Layout, Navbar } from 'nextra-theme-docs'
import type { ReactNode } from 'react'

import { Logo } from '@/components/media'
import 'nextra-theme-docs/style.css'
import './global.css'

export const metadata: Metadata = {
  description: 'Documentation for OBSMV - Native OBS Studio Music Viewer and Audio Visualizer Plugin',
  keywords: [
    'OBSMV',
    'Documentation',
    'OBS Studio',
    'OBS Plugin',
    'Music',
    'Audio Visualizer',
    'Spectrum',
    'Now Playing',
    'Glassmorphism',
  ],
  title: {
    default: 'OBSMV Docs',
    template: '%s | OBSMV Docs',
  },
}

const navbar = <Navbar logo={<Logo />} projectLink="https://github.com/StreamMusicViewer/OBSMV" />

export default async function RootLayout({ children }: { children: ReactNode }) {
  const pageMap = await getPageMap()

  return (
    <html lang="en" dir="ltr" suppressHydrationWarning>
      <Head backgroundColor={{ dark: '#141d29' }} />
      <body>
        <Layout
          navbar={navbar}
          pageMap={pageMap}
          docsRepositoryBase="https://github.com/StreamMusicViewer/OBSMV/tree/main/docs"
          sidebar={{ defaultMenuCollapseLevel: 1 }}
        >
          {children}
        </Layout>
      </body>
    </html>
  )
}
