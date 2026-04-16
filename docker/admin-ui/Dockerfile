# ============================================================================
# ThemisDB Admin UI - Sidecar Container
# ============================================================================
# Lightweight nginx container serving the ThemisDB web admin interface.
# Proxies /api/ to the ThemisDB backend to avoid CORS issues.
#
# Build:
#   docker build -t themisdb-admin-ui docker/admin-ui/
#
# Run (standalone, adjust THEMISDB_API_URL as needed):
#   docker run -p 18766:80 \
#     -e THEMISDB_API_URL=http://themisdb:8080 \
#     themisdb-admin-ui
# ============================================================================

FROM nginx:1.25-alpine

LABEL org.opencontainers.image.title="ThemisDB Admin UI"
LABEL org.opencontainers.image.description="Web-based admin interface for ThemisDB"
LABEL org.opencontainers.image.version="1.0.0"

# Copy nginx configuration
COPY nginx.conf /etc/nginx/conf.d/default.conf

# Copy the static web application
COPY app/ /usr/share/nginx/html/

# Remove default nginx index page
RUN rm -f /usr/share/nginx/html/50x.html

EXPOSE 80

HEALTHCHECK --interval=30s --timeout=5s --retries=3 \
    CMD wget --quiet --tries=1 --spider http://localhost/health-ui || exit 1
