/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            MeshGenerator.cs                                   ║
  Version:         0.0.25                                             ║
  Last Modified:   2026-02-22 08:22:14                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   96.0/100                                       ║
    • Total Lines:     150                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using System;
using System.Collections.Generic;

namespace Themis.DocumentManager.Services.DirectX;

/// <summary>
/// Simple Mesh Generator (keine externe Abhängigkeiten)
/// </summary>
public static class MeshGenerator
{
    public struct SimpleVertex
    {
        public float X, Y, Z;
        public float R, G, B, A;
        public float NX, NY, NZ;
    }

    public static (SimpleVertex[] vertices, uint[] indices) GenerateSphereMesh(
        float radius = 1.0f,
        int segments = 16,
        int rings = 16,
        (float R, float G, float B, float A)? color = null)
    {
        var c = color ?? (0.2f, 0.6f, 1.0f, 1.0f);
        var verts = new List<SimpleVertex>();
        var inds = new List<uint>();

        for (int ring = 0; ring <= rings; ring++)
        {
            float phi = (float)(Math.PI * ring / rings);
            float sinPhi = (float)Math.Sin(phi);
            float cosPhi = (float)Math.Cos(phi);

            for (int seg = 0; seg <= segments; seg++)
            {
                float theta = (float)(2 * Math.PI * seg / segments);
                float sinTheta = (float)Math.Sin(theta);
                float cosTheta = (float)Math.Cos(theta);

                float nx = sinPhi * cosTheta;
                float ny = cosPhi;
                float nz = sinPhi * sinTheta;

                verts.Add(new SimpleVertex
                {
                    X = nx * radius,
                    Y = ny * radius,
                    Z = nz * radius,
                    R = c.R,
                    G = c.G,
                    B = c.B,
                    A = c.A,
                    NX = nx,
                    NY = ny,
                    NZ = nz
                });
            }
        }

        for (int ring = 0; ring < rings; ring++)
        {
            for (int seg = 0; seg < segments; seg++)
            {
                uint a = (uint)(ring * (segments + 1) + seg);
                uint b = a + 1;
                uint c2 = (uint)((ring + 1) * (segments + 1) + seg);
                uint d = c2 + 1;

                inds.Add(a);
                inds.Add(c2);
                inds.Add(b);

                inds.Add(b);
                inds.Add(c2);
                inds.Add(d);
            }
        }

        return (verts.ToArray(), inds.ToArray());
    }

    public static (SimpleVertex[] vertices, uint[] indices) GenerateCylinderMesh(
        float radius = 0.1f,
        float height = 1.0f,
        int segments = 8,
        (float R, float G, float B, float A)? color = null)
    {
        var c = color ?? (0.5f, 0.5f, 0.5f, 1.0f);
        var verts = new List<SimpleVertex>();
        var inds = new List<uint>();

        float halfH = height / 2.0f;

        for (int i = 0; i <= segments; i++)
        {
            float angle = (float)(2 * Math.PI * i / segments);
            float x = radius * (float)Math.Cos(angle);
            float z = radius * (float)Math.Sin(angle);
            float nx = (float)Math.Cos(angle);
            float nz = (float)Math.Sin(angle);

            verts.Add(new SimpleVertex { X = x, Y = halfH, Z = z, R = c.R, G = c.G, B = c.B, A = c.A, NX = nx, NY = 0, NZ = nz });
            verts.Add(new SimpleVertex { X = x, Y = -halfH, Z = z, R = c.R, G = c.G, B = c.B, A = c.A, NX = nx, NY = 0, NZ = nz });
        }

        for (int i = 0; i < segments; i++)
        {
            uint t1 = (uint)(i * 2);
            uint t2 = (uint)((i + 1) * 2);
            uint b1 = (uint)(i * 2 + 1);
            uint b2 = (uint)((i + 1) * 2 + 1);

            inds.Add(t1);
            inds.Add(b1);
            inds.Add(t2);

            inds.Add(t2);
            inds.Add(b1);
            inds.Add(b2);
        }

        return (verts.ToArray(), inds.ToArray());
    }
}
