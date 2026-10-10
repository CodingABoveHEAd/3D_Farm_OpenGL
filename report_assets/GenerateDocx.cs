using System;
using System.Collections.Generic;
using System.Globalization;
using System.IO;
using System.Linq;
using System.Text.RegularExpressions;
using System.Xml.Linq;
using DocumentFormat.OpenXml;
using DocumentFormat.OpenXml.Packaging;
using DocumentFormat.OpenXml.Wordprocessing;
using A = DocumentFormat.OpenXml.Drawing;
using DW = DocumentFormat.OpenXml.Drawing.Wordprocessing;
using PIC = DocumentFormat.OpenXml.Drawing.Pictures;
using WpRun = DocumentFormat.OpenXml.Wordprocessing.Run;

public static class FarmReportDocx
{
    private static MainDocumentPart mainPart;
    private static uint imageId = 1;
    private static readonly XNamespace XmlNs = "http://www.w3.org/XML/1998/namespace";

    public static void Build(string htmlPath, string docxPath)
    {
        string html = File.ReadAllText(htmlPath);
        MatchCollection matches = Regex.Matches(html, "<section\\b[\\s\\S]*?</section>", RegexOptions.IgnoreCase);
        if (matches.Count != 20)
            throw new InvalidOperationException("Expected 20 report sections, found " + matches.Count + ".");

        if (File.Exists(docxPath)) File.Delete(docxPath);
        using (WordprocessingDocument package = WordprocessingDocument.Create(docxPath, WordprocessingDocumentType.Document))
        {
            mainPart = package.AddMainDocumentPart();
            mainPart.Document = new Document(new Body());
            AddStyles();
            AddProperties(package);
            AddFooter();

            Body body = mainPart.Document.Body;
            for (int i = 0; i < matches.Count; ++i)
            {
                string fragment = matches[i].Value.Replace("&nbsp;", "&#160;");
                XElement section = XElement.Parse(fragment, LoadOptions.PreserveWhitespace);
                foreach (XNode node in section.Nodes()) AddBlock(body, node, 0);
                if (i + 1 < matches.Count)
                    body.Append(new Paragraph(new WpRun(new Break { Type = BreakValues.Page })));
            }

            body.Append(CreateSectionProperties());
            mainPart.Document.Save();
        }
    }

    private static void AddStyles()
    {
        StyleDefinitionsPart stylesPart = mainPart.AddNewPart<StyleDefinitionsPart>();
        Styles styles = new Styles();
        Style normal = new Style { Type = StyleValues.Paragraph, StyleId = "Normal", Default = true };
        normal.Append(new StyleName { Val = "Normal" });
        normal.Append(new StyleRunProperties(
            new RunFonts { Ascii = "Calibri", HighAnsi = "Calibri", EastAsia = "Calibri", ComplexScript = "Calibri" },
            new FontSize { Val = "14" }, new FontSizeComplexScript { Val = "14" },
            new Color { Val = "26352C" }));
        normal.Append(new StyleParagraphProperties(
            new SpacingBetweenLines { Before = "0", After = "45", Line = "168", LineRule = LineSpacingRuleValues.Exact }));
        styles.Append(normal);
        stylesPart.Styles = styles;
        stylesPart.Styles.Save();
    }

    private static void AddProperties(WordprocessingDocument package)
    {
        package.PackageProperties.Title = "Animated 3D Farm Scene with Lighting and Transformations";
        package.PackageProperties.Subject = "3D_farm_OpenGL project report";
        package.PackageProperties.Creator = "Niloy Chowdhury";
        package.PackageProperties.LastModifiedBy = "Niloy Chowdhury";
        package.PackageProperties.Created = DateTime.UtcNow;
        package.PackageProperties.Modified = DateTime.UtcNow;
    }

    private static void AddFooter()
    {
        FooterPart footerPart = mainPart.AddNewPart<FooterPart>();
        Paragraph p = NewParagraph(12, "607267", JustificationValues.Center, 0, 0, false);
        p.Append(NewRun("Niloy Chowdhury  |  Roll 2107117  |  PAGE ", false, false, 12, "607267"));
        p.Append(new WpRun(new FieldChar { FieldCharType = FieldCharValues.Begin }));
        p.Append(new WpRun(new FieldCode(" PAGE ")));
        p.Append(new WpRun(new FieldChar { FieldCharType = FieldCharValues.Separate }));
        p.Append(NewRun("1", false, false, 12, "607267"));
        p.Append(new WpRun(new FieldChar { FieldCharType = FieldCharValues.End }));
        p.Append(NewRun(" OF 20", true, false, 12, "607267"));
        footerPart.Footer = new Footer(p);
        footerPart.Footer.Save();
    }

    private static SectionProperties CreateSectionProperties()
    {
        string footerId = mainPart.GetIdOfPart(mainPart.FooterParts.First());
        return new SectionProperties(
            new FooterReference { Type = HeaderFooterValues.Default, Id = footerId },
            new PageSize { Width = 11906, Height = 16838 },
            new PageMargin { Top = 650, Right = 720, Bottom = 720, Left = 720, Header = 300, Footer = 300, Gutter = 0 });
    }

    private static void AddBlock(OpenXmlCompositeElement parent, XNode node, int depth)
    {
        XText textNode = node as XText;
        if (textNode != null)
        {
            string text = Normalize(textNode.Value);
            if (text.Length > 0) parent.Append(TextParagraph(text, 14, false, "26352C"));
            return;
        }

        XElement element = node as XElement;
        if (element == null) return;
        string name = element.Name.LocalName.ToLowerInvariant();
        string cssClass = (string)element.Attribute("class") ?? "";

        if (cssClass.IndexOf("footer", StringComparison.OrdinalIgnoreCase) >= 0) return;
        if (name == "img")
        {
            parent.Append(ImageParagraph(element));
            return;
        }
        if (name == "table")
        {
            parent.Append(CreateTable(element));
            return;
        }
        if (name == "ul" || name == "ol")
        {
            int number = 1;
            foreach (XElement li in element.Elements().Where(e => e.Name.LocalName.Equals("li", StringComparison.OrdinalIgnoreCase)))
            {
                string prefix = name == "ol" ? (number++).ToString(CultureInfo.InvariantCulture) + ". " : "• ";
                Paragraph p = NewParagraph(13, "26352C", JustificationValues.Left, 0, 35, false);
                p.Append(NewRun(prefix + Normalize(li.Value), false, false, 13, "26352C"));
                parent.Append(p);
            }
            return;
        }
        if (name == "h1")
        {
            Paragraph p = NewParagraph(32, "174F35", JustificationValues.Left, 0, 80, true);
            AppendInline(p, element, true, false, false, false, 32, "174F35");
            parent.Append(p); return;
        }
        if (name == "h2")
        {
            Paragraph p = NewParagraph(19, "356B49", JustificationValues.Left, 70, 30, true);
            AppendInline(p, element, true, false, false, false, 19, "356B49");
            parent.Append(p); return;
        }
        if (name == "p")
        {
            Paragraph p = NewParagraph(14, "26352C", JustificationValues.Both, 0, 45, false);
            AppendInline(p, element, false, false, false, false, 14, "26352C");
            parent.Append(p); return;
        }
        if (name == "div")
        {
            if (cssClass.IndexOf("running", StringComparison.OrdinalIgnoreCase) >= 0)
            {
                Paragraph p = NewParagraph(11, "52705C", JustificationValues.Left, 0, 70, true);
                p.Append(NewRun(Normalize(element.Value), true, false, 11, "52705C"));
                p.ParagraphProperties.Append(new ParagraphBorders(new BottomBorder { Val = BorderValues.Single, Color = "93A99A", Size = 4 }));
                parent.Append(p); return;
            }
            if (cssClass.IndexOf("cover-title", StringComparison.OrdinalIgnoreCase) >= 0)
            {
                Paragraph p = NewParagraph(40, "164D34", JustificationValues.Left, 0, 80, true);
                AppendInline(p, element, true, false, false, false, 40, "164D34"); parent.Append(p); return;
            }
            if (cssClass.IndexOf("cover-kicker", StringComparison.OrdinalIgnoreCase) >= 0 || cssClass.IndexOf("cover-subtitle", StringComparison.OrdinalIgnoreCase) >= 0)
            {
                Paragraph p = NewParagraph(15, "4E705B", JustificationValues.Left, 0, 55, true);
                AppendInline(p, element, true, false, false, false, 15, "4E705B"); parent.Append(p); return;
            }
            if (cssClass.IndexOf("cover-rule", StringComparison.OrdinalIgnoreCase) >= 0) return;
            if (cssClass.IndexOf("caption", StringComparison.OrdinalIgnoreCase) >= 0)
            {
                Paragraph p = NewParagraph(11, "57665D", JustificationValues.Center, 0, 35, false);
                AppendInline(p, element, false, true, false, false, 11, "57665D"); parent.Append(p); return;
            }
            if (cssClass.IndexOf("formula", StringComparison.OrdinalIgnoreCase) >= 0 || cssClass.IndexOf("callout", StringComparison.OrdinalIgnoreCase) >= 0 || cssClass.IndexOf("signoff", StringComparison.OrdinalIgnoreCase) >= 0)
            {
                Paragraph p = NewParagraph(13, "315F42", JustificationValues.Center, 35, 55, false);
                AppendInline(p, element, false, false, false, false, 13, "315F42");
                p.ParagraphProperties.Append(new Shading { Val = ShadingPatternValues.Clear, Fill = "EDF4EF" });
                parent.Append(p); return;
            }
            bool hasBlockChildren = element.Elements().Any(e => IsBlock(e.Name.LocalName));
            if (!hasBlockChildren && Normalize(element.Value).Length > 0)
            {
                Paragraph p = NewParagraph(14, "26352C", JustificationValues.Left, 0, 40, false);
                AppendInline(p, element, false, false, false, false, 14, "26352C"); parent.Append(p); return;
            }
            foreach (XNode child in element.Nodes()) AddBlock(parent, child, depth + 1);
            return;
        }
        foreach (XNode child in element.Nodes()) AddBlock(parent, child, depth + 1);
    }

    private static bool IsBlock(string name)
    {
        string n = name.ToLowerInvariant();
        return n == "div" || n == "p" || n == "h1" || n == "h2" || n == "table" || n == "ul" || n == "ol" || n == "img";
    }

    private static Table CreateTable(XElement source)
    {
        Table table = new Table();
        table.Append(new TableProperties(
            new TableWidth { Type = TableWidthUnitValues.Pct, Width = "5000" },
            new TableBorders(
                new TopBorder { Val = BorderValues.Single, Color = "AEBDB2", Size = 3 },
                new LeftBorder { Val = BorderValues.Single, Color = "AEBDB2", Size = 3 },
                new BottomBorder { Val = BorderValues.Single, Color = "AEBDB2", Size = 3 },
                new RightBorder { Val = BorderValues.Single, Color = "AEBDB2", Size = 3 },
                new InsideHorizontalBorder { Val = BorderValues.Single, Color = "C8D2CA", Size = 2 },
                new InsideVerticalBorder { Val = BorderValues.Single, Color = "C8D2CA", Size = 2 }),
            new TableCellMarginDefault(
                new TopMargin { Width = "30", Type = TableWidthUnitValues.Dxa },
                new TableCellLeftMargin { Width = 45, Type = TableWidthValues.Dxa },
                new BottomMargin { Width = "30", Type = TableWidthUnitValues.Dxa },
                new TableCellRightMargin { Width = 45, Type = TableWidthValues.Dxa })));

        foreach (XElement rowSource in source.Descendants().Where(e => e.Name.LocalName.Equals("tr", StringComparison.OrdinalIgnoreCase)))
        {
            TableRow row = new TableRow();
            foreach (XElement cellSource in rowSource.Elements().Where(e => e.Name.LocalName.Equals("td", StringComparison.OrdinalIgnoreCase) || e.Name.LocalName.Equals("th", StringComparison.OrdinalIgnoreCase)))
            {
                bool header = cellSource.Name.LocalName.Equals("th", StringComparison.OrdinalIgnoreCase);
                TableCell cell = new TableCell();
                cell.Append(new TableCellProperties(
                    new TableCellVerticalAlignment { Val = TableVerticalAlignmentValues.Top },
                    new Shading { Val = ShadingPatternValues.Clear, Fill = header ? "285F42" : "FFFFFF" }));
                List<XElement> images = cellSource.Descendants().Where(e => e.Name.LocalName.Equals("img", StringComparison.OrdinalIgnoreCase)).ToList();
                foreach (XElement image in images) cell.Append(ImageParagraph(image));
                string cellText = Normalize(String.Concat(cellSource.DescendantNodes().OfType<XText>().Select(t => t.Value)));
                if (cellText.Length > 0)
                {
                    Paragraph p = NewParagraph(12, header ? "FFFFFF" : "26352C", JustificationValues.Left, 0, 0, false);
                    p.Append(NewRun(cellText, header, false, 12, header ? "FFFFFF" : "26352C"));
                    cell.Append(p);
                }
                if (!cell.Elements<Paragraph>().Any()) cell.Append(new Paragraph());
                row.Append(cell);
            }
            if (row.Elements<TableCell>().Any()) table.Append(row);
        }
        return table;
    }

    private static Paragraph ImageParagraph(XElement image)
    {
        string source = (string)image.Attribute("src") ?? "";
        string path = new Uri(source).LocalPath;
        double widthCm = 7.0;
        string style = (string)image.Attribute("style") ?? "";
        Match match = Regex.Match(style, "width:([0-9.]+)cm", RegexOptions.IgnoreCase);
        if (match.Success) widthCm = Double.Parse(match.Groups[1].Value, CultureInfo.InvariantCulture);
        widthCm *= 0.54;
        if (widthCm > 9.6) widthCm = 9.6;
        if (widthCm < 2.5) widthCm = 2.5;

        double ratio = 16.0 / 9.0;
        using (System.Drawing.Image bitmap = System.Drawing.Image.FromFile(path))
            ratio = (double)bitmap.Width / bitmap.Height;
        long cx = (long)(widthCm / 2.54 * 914400.0);
        long cy = (long)(cx / ratio);

        ImagePart part = mainPart.AddImagePart(ImagePartType.Png);
        using (FileStream stream = File.OpenRead(path)) part.FeedData(stream);
        string relationshipId = mainPart.GetIdOfPart(part);

        A.GraphicData graphicData = new A.GraphicData(
            new PIC.Picture(
                new PIC.NonVisualPictureProperties(
                    new PIC.NonVisualDrawingProperties { Id = 0, Name = Path.GetFileName(path) },
                    new PIC.NonVisualPictureDrawingProperties()),
                new PIC.BlipFill(new A.Blip { Embed = relationshipId }, new A.Stretch(new A.FillRectangle())),
                new PIC.ShapeProperties(
                    new A.Transform2D(new A.Offset { X = 0, Y = 0 }, new A.Extents { Cx = cx, Cy = cy }),
                    new A.PresetGeometry(new A.AdjustValueList()) { Preset = A.ShapeTypeValues.Rectangle })))
        { Uri = "http://schemas.openxmlformats.org/drawingml/2006/picture" };
        Drawing drawing = new Drawing(
            new DW.Inline(
                new DW.Extent { Cx = cx, Cy = cy },
                new DW.EffectExtent { LeftEdge = 0, TopEdge = 0, RightEdge = 0, BottomEdge = 0 },
                new DW.DocProperties { Id = imageId++, Name = "Report image" },
                new DW.NonVisualGraphicFrameDrawingProperties(new A.GraphicFrameLocks { NoChangeAspect = true }),
                new A.Graphic(graphicData))
            { DistanceFromTop = 0, DistanceFromBottom = 0, DistanceFromLeft = 0, DistanceFromRight = 0 });

        // Inline drawings are part of the paragraph's line box. An exact text-sized
        // line height clips every screenshot to a thin strip in Word/PDF, so image
        // paragraphs must be allowed to expand to the drawing's natural height.
        Paragraph p = new Paragraph(new ParagraphProperties(
            new Justification { Val = JustificationValues.Center },
            new SpacingBetweenLines { Before = "0", After = "25", Line = "240", LineRule = LineSpacingRuleValues.Auto }));
        p.Append(new WpRun(drawing));
        return p;
    }

    private static void AppendInline(Paragraph paragraph, XContainer container, bool bold, bool italic, bool superscript, bool subscript, int size, string color)
    {
        foreach (XNode child in container.Nodes())
        {
            XText text = child as XText;
            if (text != null)
            {
                string value = NormalizeInline(text.Value);
                if (value.Length > 0) paragraph.Append(NewRun(value, bold, italic, size, color, superscript, subscript));
                continue;
            }
            XElement element = child as XElement;
            if (element == null) continue;
            string name = element.Name.LocalName.ToLowerInvariant();
            if (name == "br") { paragraph.Append(new WpRun(new Break())); continue; }
            if (name == "img") { continue; }
            bool nextBold = bold || name == "b" || name == "strong";
            bool nextItalic = italic || name == "i" || name == "em";
            int nextSize = name == "code" ? Math.Max(11, size - 1) : size;
            string nextColor = name == "code" ? "823F24" : color;
            AppendInline(paragraph, element, nextBold, nextItalic, superscript || name == "sup", subscript || name == "sub", nextSize, nextColor);
        }
    }

    private static Paragraph TextParagraph(string text, int size, bool bold, string color)
    {
        Paragraph p = NewParagraph(size, color, JustificationValues.Left, 0, 35, false);
        p.Append(NewRun(text, bold, false, size, color));
        return p;
    }

    private static Paragraph NewParagraph(int size, string color, JustificationValues alignment, int before, int after, bool keepNext)
    {
        ParagraphProperties properties = new ParagraphProperties(
            new Justification { Val = alignment },
            new SpacingBetweenLines { Before = before.ToString(CultureInfo.InvariantCulture), After = after.ToString(CultureInfo.InvariantCulture), Line = Math.Max(150, size * 12).ToString(CultureInfo.InvariantCulture), LineRule = LineSpacingRuleValues.Exact });
        if (keepNext) properties.Append(new KeepNext());
        return new Paragraph(properties);
    }

    private static WpRun NewRun(string text, bool bold, bool italic, int size, string color, bool superscript = false, bool subscript = false)
    {
        RunProperties properties = new RunProperties(
            new RunFonts { Ascii = "Calibri", HighAnsi = "Calibri", EastAsia = "Calibri", ComplexScript = "Calibri" },
            new FontSize { Val = size.ToString(CultureInfo.InvariantCulture) },
            new FontSizeComplexScript { Val = size.ToString(CultureInfo.InvariantCulture) },
            new Color { Val = color });
        if (bold) properties.Append(new Bold());
        if (italic) properties.Append(new Italic());
        if (superscript) properties.Append(new VerticalTextAlignment { Val = VerticalPositionValues.Superscript });
        if (subscript) properties.Append(new VerticalTextAlignment { Val = VerticalPositionValues.Subscript });
        Text value = new Text(text) { Space = SpaceProcessingModeValues.Preserve };
        return new WpRun(properties, value);
    }

    private static string Normalize(string value)
    {
        return Regex.Replace(value ?? "", "\\s+", " ").Trim();
    }

    private static string NormalizeInline(string value)
    {
        return Regex.Replace(value ?? "", "\\s+", " ");
    }
}
