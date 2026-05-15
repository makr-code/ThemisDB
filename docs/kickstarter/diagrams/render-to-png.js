#!/usr/bin/env node
/**
 * Render Mermaid diagrams to PNG using Puppeteer
 * Usage: node render-to-png.js
 */

const fs = require('fs');
const path = require('path');
const puppeteer = require('puppeteer');

const diagramDir = __dirname;
const outputDir = path.join(diagramDir, 'png');

// Ensure output directory exists
if (!fs.existsSync(outputDir)) {
  fs.mkdirSync(outputDir, { recursive: true });
  console.log(`[OK] Created output directory: ${outputDir}`);
}

// Find all .mmd files
const mmdFiles = fs.readdirSync(diagramDir).filter(f => f.endsWith('.mmd'));

if (mmdFiles.length === 0) {
  console.log('[WARN] No .mmd files found');
  process.exit(1);
}

console.log(`Converting ${mmdFiles.length} Mermaid diagrams to PNG...`);

(async () => {
  let browser;
  try {
    browser = await puppeteer.launch({
      headless: 'new',
      defaultViewport: null
    });

    for (const file of mmdFiles) {
      const inputPath = path.join(diagramDir, file);
      const outputPath = path.join(outputDir, file.replace('.mmd', '.png'));
      
      console.log(`  Converting: ${file}...`);
      
      try {
        const mermaidCode = fs.readFileSync(inputPath, 'utf8');
        
        // Create HTML with mermaid diagram
        const html = `
          <!DOCTYPE html>
          <html>
          <head>
            <meta charset="utf-8">
            <script src="https://cdn.jsdelivr.net/npm/mermaid@10/dist/mermaid.min.js"></script>
            <style>
              body { margin: 0; padding: 20px; background: white; }
              svg { display: block; margin: 0 auto; }
            </style>
          </head>
          <body>
            <div class="mermaid">
${mermaidCode}
            </div>
            <script>
              mermaid.initialize({ startOnLoad: true, theme: 'default' });
              mermaid.contentLoaded();
            </script>
          </body>
          </html>
        `;
        
        // Save HTML to temp file
        const tempHtml = path.join(outputDir, `temp_${file.replace('.mmd', '.html')}`);
        fs.writeFileSync(tempHtml, html);
        
        // Open in puppeteer and screenshot
        const page = await browser.newPage();
        await page.goto(`file://${tempHtml}`, { waitUntil: 'networkidle0', timeout: 30000 });
        
        // Wait for mermaid to render
        await page.waitForTimeout(2000);
        
        // Get the SVG element dimensions
        const svgDimensions = await page.evaluate(() => {
          const svg = document.querySelector('svg');
          if (svg) {
            return {
              width: svg.clientWidth || 1200,
              height: svg.clientHeight || 800
            };
          }
          return { width: 1200, height: 800 };
        });
        
        // Set viewport to match SVG
        await page.setViewport({
          width: Math.ceil(svgDimensions.width) + 40,
          height: Math.ceil(svgDimensions.height) + 40
        });
        
        // Take screenshot
        await page.screenshot({ 
          path: outputPath, 
          fullPage: true,
          type: 'png'
        });
        
        await page.close();
        
        // Clean up temp HTML
        fs.unlinkSync(tempHtml);
        
        console.log(`    [OK] Saved: ${outputPath}`);
      } catch (err) {
        console.log(`    [ERROR] Error converting ${file}: ${err.message}`);
      }
    }
    
    console.log('[DONE] Conversion complete. PNG files saved to:', outputDir);
  } catch (err) {
    console.error('[FATAL] Error:', err);
    process.exit(1);
  } finally {
    if (browser) {
      await browser.close();
    }
  }
})();
