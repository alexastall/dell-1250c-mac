// Draws an A4 test page (text, colour patches, gradients, hairlines) to a PDF.
import CoreGraphics
import CoreText
import Foundation

let out = CommandLine.arguments.count > 1 ? CommandLine.arguments[1] : "testpage.pdf"
var box = CGRect(x: 0, y: 0, width: 595, height: 842)
let ctx = CGContext(URL(fileURLWithPath: out) as CFURL, mediaBox: &box, nil)!
ctx.beginPDFPage(nil)

func text(_ s: String, _ x: CGFloat, _ y: CGFloat, _ size: CGFloat) {
    let font = CTFontCreateWithName("Helvetica" as CFString, size, nil)
    let attr = NSAttributedString(string: s, attributes: [.init(kCTFontAttributeName as String): font])
    ctx.textPosition = CGPoint(x: x, y: y)
    CTLineDraw(CTLineCreateWithAttributedString(attr), ctx)
}

text("Dell 1250c - macOS HBPL v1 driver test page", 40, 790, 20)
text("The quick brown fox jumps over the lazy dog. 0123456789", 40, 760, 12)
text("Small 6pt text should still be crisp and readable.", 40, 745, 6)

let patches: [(CGFloat, CGFloat, CGFloat)] = [(0,1,1),(1,0,1),(1,1,0),(0,0,0),(1,0,0),(0,1,0),(0,0,1),(0.5,0.5,0.5)]
for (i, c) in patches.enumerated() {
    ctx.setFillColor(red: c.0, green: c.1, blue: c.2, alpha: 1)
    ctx.fill(CGRect(x: 40 + CGFloat(i) * 64, y: 640, width: 56, height: 80))
}
let cs = CGColorSpaceCreateDeviceRGB()
let rows: [[CGFloat]] = [[1,1,1,1, 0,0,0,1], [1,0,0,1, 0,0,1,1], [1,1,0,1, 0,1,1,1]]
for (i, comps) in rows.enumerated() {
    let g = CGGradient(colorSpace: cs, colorComponents: comps, locations: [0, 1], count: 2)!
    let y = 560 - CGFloat(i) * 70
    ctx.saveGState(); ctx.clip(to: CGRect(x: 40, y: y, width: 515, height: 56))
    ctx.drawLinearGradient(g, start: CGPoint(x: 40, y: 0), end: CGPoint(x: 555, y: 0), options: [])
    ctx.restoreGState()
}
ctx.setStrokeColor(gray: 0, alpha: 1)
for i in 0..<40 {
    ctx.setLineWidth(CGFloat(i % 4 + 1) * 0.25)
    ctx.move(to: CGPoint(x: 40 + CGFloat(i) * 13, y: 200)); ctx.addLine(to: CGPoint(x: 40 + CGFloat(i) * 13, y: 340))
    ctx.strokePath()
}
ctx.setStrokeColor(red: 1, green: 0, blue: 0, alpha: 1); ctx.setLineWidth(1)
ctx.stroke(CGRect(x: 14, y: 14, width: 567, height: 814))
text("Red frame is ~5 mm from the paper edge", 40, 60, 10)
ctx.endPDFPage()
ctx.closePDF()
