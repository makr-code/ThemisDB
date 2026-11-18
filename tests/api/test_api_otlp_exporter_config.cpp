#include <gtest/gtest.h>

#include <stdexcept>

#include "api/otlp_exporter.h"

using namespace themis::api;

TEST(OtlpExporterConfigValidationTest, AcceptsDefaultsAndDocumentedBounds)
{
    OtlpExporter exporter;
    EXPECT_EQ(exporter.config().max_queue_size, 8192u);

    OtlpExporterConfig config;
    config.timeout_ms = 60000;
    config.max_export_retries = 10;
    config.retry_initial_delay_ms = 30000;
    EXPECT_NO_THROW(OtlpExporter bounded_exporter(config));
}

TEST(OtlpExporterConfigValidationTest, RejectsZeroQueueAndBatchSizes)
{
    OtlpExporterConfig config;
    config.max_queue_size = 0;
    EXPECT_THROW(OtlpExporter exporter(config), std::invalid_argument);

    config = OtlpExporterConfig{};
    config.batch_size = 0;
    EXPECT_THROW(OtlpExporter exporter(config), std::invalid_argument);
}

TEST(OtlpExporterConfigValidationTest, RejectsUnboundedOrInvalidTimingSettings)
{
    OtlpExporterConfig config;
    config.timeout_ms = 0;
    EXPECT_THROW(OtlpExporter exporter(config), std::invalid_argument);

    config = OtlpExporterConfig{};
    config.timeout_ms = 60001;
    EXPECT_THROW(OtlpExporter exporter(config), std::invalid_argument);

    config = OtlpExporterConfig{};
    config.flush_interval_ms = 0;
    EXPECT_THROW(OtlpExporter exporter(config), std::invalid_argument);

    config = OtlpExporterConfig{};
    config.retry_initial_delay_ms = 0;
    EXPECT_THROW(OtlpExporter exporter(config), std::invalid_argument);

    config = OtlpExporterConfig{};
    config.retry_initial_delay_ms = 30001;
    EXPECT_THROW(OtlpExporter exporter(config), std::invalid_argument);
}

TEST(OtlpExporterConfigValidationTest, RejectsRetryCountsOutsideBound)
{
    OtlpExporterConfig config;
    config.max_export_retries = -1;
    EXPECT_THROW(OtlpExporter exporter(config), std::invalid_argument);

    config.max_export_retries = 11;
    EXPECT_THROW(OtlpExporter exporter(config), std::invalid_argument);
}
