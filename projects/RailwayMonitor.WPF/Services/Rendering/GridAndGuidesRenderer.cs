/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            GridAndGuidesRenderer.cs                           ║
  Version:         0.0.32                                             ║
  Last Modified:   2026-02-23 03:57:46                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   99.0/100                                       ║
    • Total Lines:     651                                            ║
    • Open Issues:     TODOs: 2, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using System;
using System.Collections.Generic;
using System.Numerics;

namespace RailwayMonitor.WPF.Services.Rendering
{
    /// <summary>
    /// Renderer für dynamische Grid-Linien, Achsen und Hilfslinien in der 3D-Szene
    /// Dynamic grid, coordinate axes, and guide lines for 3D map orientation
    /// </summary>
    public class GridAndGuidesRenderer
    {
        #region Grid Configuration

        public class GridConfig
        {
            /// <summary>Grid-Typ</summary>
            public GridType Type { get; set; } = GridType.Cartesian;

            /// <summary>Grid-Größe in Metern (z.B. 1000m = 1km)</summary>
            public float CellSize { get; set; } = 1000f; // 1 km default

            /// <summary>Grid-Ausdehnung in Zellen (z.B. 50 = 50km × 50km bei 1km Zellgröße)</summary>
            public int Extent { get; set; } = 50;

            /// <summary>Haupt-Grid Farbe (dicke Linien alle N Zellen)</summary>
            public Color MajorGridColor { get; set; } = new Color(100, 100, 100, 200); // Grau, halbtransparent

            /// <summary>Unter-Grid Farbe (dünne Linien)</summary>
            public Color MinorGridColor { get; set; } = new Color(80, 80, 80, 100); // Grau, transparenter

            /// <summary>Haupt-Grid Intervall (z.B. 5 = jede 5. Linie ist dicker)</summary>
            public int MajorInterval { get; set; } = 5;

            /// <summary>Linienbreite für Haupt-Grid</summary>
            public float MajorLineWidth { get; set; } = 2.0f;

            /// <summary>Linienbreite für Unter-Grid</summary>
            public float MinorLineWidth { get; set; } = 0.5f;

            /// <summary>Höhe über Terrain (0 = auf Terrain, 1 = 1m über Terrain)</summary>
            public float HeightOffset { get; set; } = 0.1f;

            /// <summary>Sichtbarkeit basierend auf Kamera-Distanz</summary>
            public bool FadeWithDistance { get; set; } = true;

            /// <summary>Maximale Sichtdistanz in Metern</summary>
            public float MaxVisibleDistance { get; set; } = 50000f; // 50 km
        }

        public enum GridType
        {
            Cartesian,      // Rechteckiges Raster (X/Y)
            Geographic,     // Lat/Lon Linien (WGS84)
            UTM,            // UTM Zone Grid
            Polar           // Polar-Koordinaten (Kreise + Radien)
        }

        #endregion

        #region Coordinate Axes

        public class AxisConfig
        {
            /// <summary>Achsen-Länge in Metern</summary>
            public float Length { get; set; } = 10000f; // 10 km

            /// <summary>Achsen-Breite (Liniendicke)</summary>
            public float LineWidth { get; set; } = 3.0f;

            /// <summary>X-Achse Farbe (Ost/West) - typisch Rot</summary>
            public Color XAxisColor { get; set; } = new Color(255, 0, 0, 255); // Rot

            /// <summary>Y-Achse Farbe (Höhe) - typisch Grün</summary>
            public Color YAxisColor { get; set; } = new Color(0, 255, 0, 255); // Grün

            /// <summary>Z-Achse Farbe (Nord/Süd) - typisch Blau</summary>
            public Color ZAxisColor { get; set; } = new Color(0, 0, 255, 255); // Blau

            /// <summary>Achsen-Labels anzeigen</summary>
            public bool ShowLabels { get; set; } = true;

            /// <summary>Achsen-Pfeile an Enden</summary>
            public bool ShowArrowHeads { get; set; } = true;

            /// <summary>Pfeilkopf-Größe in Metern</summary>
            public float ArrowHeadSize { get; set; } = 500f;

            /// <summary>Position der Achsen (Origin)</summary>
            public Vector3 Origin { get; set; } = Vector3.Zero;
        }

        #endregion

        #region Measurement Tools

        public class RulerConfig
        {
            /// <summary>Lineal-Typ</summary>
            public RulerType Type { get; set; } = RulerType.Distance;

            /// <summary>Start-Punkt (Welt-Koordinaten)</summary>
            public Vector3 StartPoint { get; set; }

            /// <summary>End-Punkt (Welt-Koordinaten)</summary>
            public Vector3 EndPoint { get; set; }

            /// <summary>Farbe der Mess-Linie</summary>
            public Color LineColor { get; set; } = new Color(255, 255, 0, 255); // Gelb

            /// <summary>Linienbreite</summary>
            public float LineWidth { get; set; } = 2.0f;

            /// <summary>Text-Farbe für Beschriftung</summary>
            public Color TextColor { get; set; } = new Color(255, 255, 255, 255); // Weiß

            /// <summary>Markierungen alle N Meter</summary>
            public float TickInterval { get; set; } = 100f; // Alle 100m eine Markierung

            /// <summary>Markierungs-Größe</summary>
            public float TickSize { get; set; } = 50f;
        }

        public enum RulerType
        {
            Distance,       // Einfache Distanz-Messung
            Elevation,      // Höhenunterschied
            Gradient,       // Steigung in %
            Area,           // Flächen-Berechnung (Polygon)
            Volume          // Volumen-Berechnung (3D)
        }

        #endregion

        #region Rendering Data Structures

        public class GridLine
        {
            public Vector3 Start { get; set; }
            public Vector3 End { get; set; }
            public Color Color { get; set; }
            public float Width { get; set; }
            public bool IsMajor { get; set; }
        }

        public class AxisArrow
        {
            public Vector3 Start { get; set; }
            public Vector3 End { get; set; }
            public Vector3 Direction { get; set; }
            public Color Color { get; set; }
            public float Width { get; set; }
            public string Label { get; set; }
        }

        #endregion

        #region Grid Generation

        /// <summary>
        /// Generiert dynamisches Grid basierend auf Kamera-Position und Konfiguration
        /// </summary>
        public List<GridLine> GenerateGrid(GridConfig config, Vector3 cameraPosition)
        {
            var lines = new List<GridLine>();

            switch (config.Type)
            {
                case GridType.Cartesian:
                    lines = GenerateCartesianGrid(config, cameraPosition);
                    break;

                case GridType.Geographic:
                    lines = GenerateGeographicGrid(config, cameraPosition);
                    break;

                case GridType.UTM:
                    lines = GenerateUtmGrid(config, cameraPosition);
                    break;

                case GridType.Polar:
                    lines = GeneratePolarGrid(config, cameraPosition);
                    break;
            }

            // Fade mit Distanz wenn aktiviert
            if (config.FadeWithDistance)
            {
                ApplyDistanceFade(lines, cameraPosition, config.MaxVisibleDistance);
            }

            return lines;
        }

        private List<GridLine> GenerateCartesianGrid(GridConfig config, Vector3 cameraPosition)
        {
            var lines = new List<GridLine>();

            // Grid zentriert um Kamera-Position (auf nächste Zelle gerundet)
            var centerX = (int)(cameraPosition.X / config.CellSize) * config.CellSize;
            var centerZ = (int)(cameraPosition.Z / config.CellSize) * config.CellSize;

            var halfExtent = config.Extent / 2;
            var minX = centerX - halfExtent * config.CellSize;
            var maxX = centerX + halfExtent * config.CellSize;
            var minZ = centerZ - halfExtent * config.CellSize;
            var maxZ = centerZ + halfExtent * config.CellSize;

            // Vertikale Linien (parallel zu Z-Achse, Nord-Süd)
            for (int i = -halfExtent; i <= halfExtent; i++)
            {
                var x = centerX + i * config.CellSize;
                var isMajor = (i % config.MajorInterval == 0);

                lines.Add(new GridLine
                {
                    Start = new Vector3(x, config.HeightOffset, minZ),
                    End = new Vector3(x, config.HeightOffset, maxZ),
                    Color = isMajor ? config.MajorGridColor : config.MinorGridColor,
                    Width = isMajor ? config.MajorLineWidth : config.MinorLineWidth,
                    IsMajor = isMajor
                });
            }

            // Horizontale Linien (parallel zu X-Achse, Ost-West)
            for (int i = -halfExtent; i <= halfExtent; i++)
            {
                var z = centerZ + i * config.CellSize;
                var isMajor = (i % config.MajorInterval == 0);

                lines.Add(new GridLine
                {
                    Start = new Vector3(minX, config.HeightOffset, z),
                    End = new Vector3(maxX, config.HeightOffset, z),
                    Color = isMajor ? config.MajorGridColor : config.MinorGridColor,
                    Width = isMajor ? config.MajorLineWidth : config.MinorLineWidth,
                    IsMajor = isMajor
                });
            }

            return lines;
        }

        private List<GridLine> GenerateGeographicGrid(GridConfig config, Vector3 cameraPosition)
        {
            // TODO: Lat/Lon Linien (erfordert Koordinaten-Transformation)
            // Für Deutschland: Lat 47-55°N, Lon 6-15°E
            // Alle 0.5° oder 1° Linien
            return new List<GridLine>();
        }

        private List<GridLine> GenerateUtmGrid(GridConfig config, Vector3 cameraPosition)
        {
            // TODO: UTM Zone Grid (100km × 100km Quadrate)
            // Deutschland: Zone 32N und 33N
            return new List<GridLine>();
        }

        private List<GridLine> GeneratePolarGrid(GridConfig config, Vector3 cameraPosition)
        {
            var lines = new List<GridLine>();

            // Konzentrische Kreise
            var numCircles = config.Extent / 2;
            for (int i = 1; i <= numCircles; i++)
            {
                var radius = i * config.CellSize;
                var isMajor = (i % config.MajorInterval == 0);
                var circle = GenerateCircle(cameraPosition, radius, config.HeightOffset, 64);

                for (int j = 0; j < circle.Count - 1; j++)
                {
                    lines.Add(new GridLine
                    {
                        Start = circle[j],
                        End = circle[j + 1],
                        Color = isMajor ? config.MajorGridColor : config.MinorGridColor,
                        Width = isMajor ? config.MajorLineWidth : config.MinorLineWidth,
                        IsMajor = isMajor
                    });
                }
            }

            // Radiale Linien (alle 15°)
            var numRadials = 24; // Alle 15°
            for (int i = 0; i < numRadials; i++)
            {
                var angle = (float)(i * 2 * Math.PI / numRadials);
                var maxRadius = numCircles * config.CellSize;

                lines.Add(new GridLine
                {
                    Start = new Vector3(cameraPosition.X, config.HeightOffset, cameraPosition.Z),
                    End = new Vector3(
                        cameraPosition.X + maxRadius * (float)Math.Cos(angle),
                        config.HeightOffset,
                        cameraPosition.Z + maxRadius * (float)Math.Sin(angle)
                    ),
                    Color = config.MinorGridColor,
                    Width = config.MinorLineWidth,
                    IsMajor = (i % 2 == 0) // Jede 2. Linie ist Major (30°)
                });
            }

            return lines;
        }

        private List<Vector3> GenerateCircle(Vector3 center, float radius, float height, int segments)
        {
            var points = new List<Vector3>();

            for (int i = 0; i <= segments; i++)
            {
                var angle = (float)(i * 2 * Math.PI / segments);
                points.Add(new Vector3(
                    center.X + radius * (float)Math.Cos(angle),
                    height,
                    center.Z + radius * (float)Math.Sin(angle)
                ));
            }

            return points;
        }

        #endregion

        #region Coordinate Axes

        /// <summary>
        /// Generiert 3D-Koordinaten-Achsen (X=Ost/West, Y=Höhe, Z=Nord/Süd)
        /// </summary>
        public List<AxisArrow> GenerateAxes(AxisConfig config)
        {
            var axes = new List<AxisArrow>();

            // X-Achse (Ost/West, Rot)
            axes.Add(new AxisArrow
            {
                Start = config.Origin,
                End = config.Origin + new Vector3(config.Length, 0, 0),
                Direction = Vector3.UnitX,
                Color = config.XAxisColor,
                Width = config.LineWidth,
                Label = "Ost (+X)"
            });

            // Y-Achse (Höhe, Grün)
            axes.Add(new AxisArrow
            {
                Start = config.Origin,
                End = config.Origin + new Vector3(0, config.Length, 0),
                Direction = Vector3.UnitY,
                Color = config.YAxisColor,
                Width = config.LineWidth,
                Label = "Höhe (+Y)"
            });

            // Z-Achse (Nord/Süd, Blau)
            axes.Add(new AxisArrow
            {
                Start = config.Origin,
                End = config.Origin + new Vector3(0, 0, config.Length),
                Direction = Vector3.UnitZ,
                Color = config.ZAxisColor,
                Width = config.LineWidth,
                Label = "Nord (+Z)"
            });

            return axes;
        }

        /// <summary>
        /// Generiert Pfeilkopf für Achse
        /// </summary>
        public List<Vector3> GenerateArrowHead(Vector3 tipPosition, Vector3 direction, float size)
        {
            // Pfeilkopf als Kegel (8 Segmente)
            var vertices = new List<Vector3>();

            // Basis des Pfeilkopfes (etwas zurückversetzt von Spitze)
            var baseCenter = tipPosition - direction * size;

            // Senkrecht zu direction stehender Vektor
            var perpendicular1 = Vector3.Normalize(Vector3.Cross(direction, Vector3.UnitY));
            if (perpendicular1.Length() < 0.1f)
            {
                perpendicular1 = Vector3.Normalize(Vector3.Cross(direction, Vector3.UnitX));
            }
            var perpendicular2 = Vector3.Cross(direction, perpendicular1);

            var baseRadius = size * 0.3f;

            for (int i = 0; i < 8; i++)
            {
                var angle = (float)(i * 2 * Math.PI / 8);
                var offset = perpendicular1 * baseRadius * (float)Math.Cos(angle) +
                             perpendicular2 * baseRadius * (float)Math.Sin(angle);

                vertices.Add(baseCenter + offset);
            }

            // Spitze
            vertices.Add(tipPosition);

            return vertices;
        }

        #endregion

        #region Measurement Tools

        /// <summary>
        /// Generiert Mess-Lineal zwischen zwei Punkten
        /// </summary>
        public MeasurementResult GenerateRuler(RulerConfig config)
        {
            var result = new MeasurementResult();

            var delta = config.EndPoint - config.StartPoint;
            var distance = delta.Length();

            result.Lines.Add(new GridLine
            {
                Start = config.StartPoint,
                End = config.EndPoint,
                Color = config.LineColor,
                Width = config.LineWidth,
                IsMajor = true
            });

            // Markierungen alle tickInterval Meter
            var numTicks = (int)(distance / config.TickInterval);
            for (int i = 1; i < numTicks; i++)
            {
                var t = (i * config.TickInterval) / distance;
                var tickPos = config.StartPoint + delta * t;

                // Senkrechte Markierung
                var perpendicular = Vector3.Normalize(Vector3.Cross(delta, Vector3.UnitY)) * config.TickSize;

                result.Lines.Add(new GridLine
                {
                    Start = tickPos - perpendicular,
                    End = tickPos + perpendicular,
                    Color = config.LineColor,
                    Width = config.LineWidth * 0.5f,
                    IsMajor = false
                });
            }

            // Messwerte berechnen
            switch (config.Type)
            {
                case RulerType.Distance:
                    result.Value = distance;
                    result.Unit = "m";
                    result.Label = $"{distance:F1} m";
                    break;

                case RulerType.Elevation:
                    result.Value = Math.Abs(config.EndPoint.Y - config.StartPoint.Y);
                    result.Unit = "m";
                    result.Label = $"Δh = {result.Value:F1} m";
                    break;

                case RulerType.Gradient:
                    var horizontalDist = new Vector2(delta.X, delta.Z).Length();
                    var gradient = (config.EndPoint.Y - config.StartPoint.Y) / horizontalDist * 100;
                    result.Value = gradient;
                    result.Unit = "%";
                    result.Label = $"{gradient:F2} %";
                    break;
            }

            return result;
        }

        public class MeasurementResult
        {
            public List<GridLine> Lines { get; set; } = new List<GridLine>();
            public double Value { get; set; }
            public string Unit { get; set; }
            public string Label { get; set; }
        }

        #endregion

        #region Helper Methods

        private void ApplyDistanceFade(List<GridLine> lines, Vector3 cameraPosition, float maxDistance)
        {
            foreach (var line in lines)
            {
                var lineCenter = (line.Start + line.End) * 0.5f;
                var distToCamera = (lineCenter - cameraPosition).Length();

                if (distToCamera > maxDistance)
                {
                    // Komplett ausblenden
                    line.Color = new Color(line.Color.R, line.Color.G, line.Color.B, 0);
                }
                else
                {
                    // Fade-Out bei 80-100% der maximalen Distanz
                    var fadeStart = maxDistance * 0.8f;
                    if (distToCamera > fadeStart)
                    {
                        var fadeRatio = 1.0f - (distToCamera - fadeStart) / (maxDistance - fadeStart);
                        var originalAlpha = line.Color.A;
                        line.Color = new Color(
                            line.Color.R,
                            line.Color.G,
                            line.Color.B,
                            (byte)(originalAlpha * fadeRatio)
                        );
                    }
                }
            }
        }

        #endregion

        #region Color Helper

        public struct Color
        {
            public byte R { get; set; }
            public byte G { get; set; }
            public byte B { get; set; }
            public byte A { get; set; }

            public Color(byte r, byte g, byte b, byte a)
            {
                R = r;
                G = g;
                B = b;
                A = a;
            }
        }

        #endregion

        #region Compass Rose (Himmelsrichtungen)

        /// <summary>
        /// Generiert Kompass-Rose zur Orientierung (N/S/E/W)
        /// </summary>
        public List<GridLine> GenerateCompassRose(Vector3 position, float radius, float height)
        {
            var lines = new List<GridLine>();

            // Haupt-Himmelsrichtungen
            var directions = new[]
            {
                (Vector3.UnitX, "E", new Color(255, 200, 0, 255)),      // Ost (Gelb)
                (Vector3.UnitZ, "N", new Color(0, 150, 255, 255)),      // Nord (Blau)
                (-Vector3.UnitX, "W", new Color(255, 100, 0, 255)),     // West (Orange)
                (-Vector3.UnitZ, "S", new Color(150, 255, 150, 255))    // Süd (Grün)
            };

            foreach (var (dir, label, color) in directions)
            {
                lines.Add(new GridLine
                {
                    Start = position,
                    End = position + new Vector3(dir.X * radius, height, dir.Z * radius),
                    Color = color,
                    Width = 3.0f,
                    IsMajor = true
                });
            }

            // Kreis um Kompass
            var circle = GenerateCircle(position, radius * 0.9f, height, 32);
            for (int i = 0; i < circle.Count - 1; i++)
            {
                lines.Add(new GridLine
                {
                    Start = circle[i],
                    End = circle[i + 1],
                    Color = new Color(255, 255, 255, 200),
                    Width = 1.0f,
                    IsMajor = false
                });
            }

            return lines;
        }

        #endregion

        #region Scale Bar (Maßstab)

        /// <summary>
        /// Generiert Maßstabs-Leiste (z.B. "0 - 1 km - 2 km - 3 km")
        /// </summary>
        public List<GridLine> GenerateScaleBar(Vector3 position, float totalLength, int numSegments, float height)
        {
            var lines = new List<GridLine>();
            var segmentLength = totalLength / numSegments;

            for (int i = 0; i <= numSegments; i++)
            {
                var x = position.X + i * segmentLength;
                var tickHeight = (i % 2 == 0) ? 100f : 50f; // Abwechselnd hohe/niedrige Markierungen

                lines.Add(new GridLine
                {
                    Start = new Vector3(x, height, position.Z),
                    End = new Vector3(x, height, position.Z + tickHeight),
                    Color = new Color(0, 0, 0, 255),
                    Width = 2.0f,
                    IsMajor = (i % 2 == 0)
                });
            }

            // Verbindungs-Linie
            lines.Add(new GridLine
            {
                Start = position,
                End = new Vector3(position.X + totalLength, height, position.Z),
                Color = new Color(0, 0, 0, 255),
                Width = 3.0f,
                IsMajor = true
            });

            return lines;
        }

        #endregion
    }
}
