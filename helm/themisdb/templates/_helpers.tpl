{{/*
Expand the name of the chart.
*/}}
{{- define "themisdb.name" -}}
{{- default .Chart.Name .Values.nameOverride | trunc 63 | trimSuffix "-" }}
{{- end }}

{{/*
Create a default fully qualified app name.
We truncate at 63 chars because some Kubernetes name fields are limited to this (by the DNS naming spec).
If release name contains chart name it will be used as a full name.
*/}}
{{- define "themisdb.fullname" -}}
{{- if .Values.fullnameOverride }}
{{- .Values.fullnameOverride | trunc 63 | trimSuffix "-" }}
{{- else }}
{{- $name := default .Chart.Name .Values.nameOverride }}
{{- if contains $name .Release.Name }}
{{- .Release.Name | trunc 63 | trimSuffix "-" }}
{{- else }}
{{- printf "%s-%s" .Release.Name $name | trunc 63 | trimSuffix "-" }}
{{- end }}
{{- end }}
{{- end }}

{{/*
Create chart name and version as used by the chart label.
*/}}
{{- define "themisdb.chart" -}}
{{- printf "%s-%s" .Chart.Name .Chart.Version | replace "+" "_" | trunc 63 | trimSuffix "-" }}
{{- end }}

{{/*
Common labels
*/}}
{{- define "themisdb.labels" -}}
helm.sh/chart: {{ include "themisdb.chart" . }}
{{ include "themisdb.selectorLabels" . }}
{{- if .Chart.AppVersion }}
app.kubernetes.io/version: {{ .Chart.AppVersion | quote }}
{{- end }}
app.kubernetes.io/managed-by: {{ .Release.Service }}
{{- end }}

{{/*
Selector labels
*/}}
{{- define "themisdb.selectorLabels" -}}
app.kubernetes.io/name: {{ include "themisdb.name" . }}
app.kubernetes.io/instance: {{ .Release.Name }}
{{- end }}

{{/*
Create the name of the service account to use
*/}}
{{- define "themisdb.serviceAccountName" -}}
{{- if .Values.serviceAccount.create }}
{{- default (include "themisdb.fullname" .) .Values.serviceAccount.name }}
{{- else }}
{{- default "default" .Values.serviceAccount.name }}
{{- end }}
{{- end }}

{{/*
Render a single Grafana dashboard ConfigMap.
Arguments (list): ctx, category, filename, content, namespace, labelKey, labelValue
*/}}
{{- define "themisdb.dashboardConfigMap" -}}
{{- $ctx := index . 0 -}}
{{- $category := index . 1 -}}
{{- $filename := index . 2 -}}
{{- $content := index . 3 -}}
{{- $ns := index . 4 -}}
{{- $label := index . 5 -}}
{{- $labelVal := index . 6 -}}
apiVersion: v1
kind: ConfigMap
metadata:
  name: {{ include "themisdb.fullname" $ctx }}-dashboard-{{ $category }}-{{ $filename | replace ".json" "" | lower | replace "_" "-" | replace " " "-" }}
  namespace: {{ $ns }}
  labels:
    {{- include "themisdb.labels" $ctx | nindent 4 }}
    {{ $label }}: {{ $labelVal | quote }}
    themisdb.io/dashboard-category: {{ $category | quote }}
data:
  {{ $filename }}: |-
    {{ $content | nindent 4 }}
---
{{- end -}}
