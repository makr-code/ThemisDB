<?php
/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            class-license-renewal.php                          ║
  Version:         0.0.2                                              ║
  Last Modified:   2026-03-09 04:08:19                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     249                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb0423  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • 9d3ecaa0e  2026-02-28  Add ThemisDB Wiki Integration plugin with documentation i... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */


/**
 * License Renewal Reminder for ThemisDB
 *
 * Runs as a daily WP-Cron job (registered on plugin activation).
 * For every active license whose expiry date falls within the configured
 * reminder window, it sends one renewal reminder e-mail per day until the
 * license is renewed or expires.
 *
 * The number of days before expiry to start sending reminders is read from
 * the WordPress option `themisdb_license_renewal_reminder_days` (default: 30).
 *
 * To prevent duplicate e-mails within a single day, a meta record is kept
 * in the license `usage_data` JSON field:
 *   usage_data.renewal_reminder_sent_date  →  last date a reminder was sent
 */

if ( ! defined( 'ABSPATH' ) ) {
    exit;
}

class ThemisDB_License_Renewal {

    /**
     * Check all active licenses and send renewal reminders as needed.
     * Called daily by WP-Cron via the `themisdb_license_renewal_check` action.
     */
    public static function send_renewal_reminders() {
        global $wpdb;

        $reminder_days = (int) get_option( 'themisdb_license_renewal_reminder_days', 30 );
        if ( $reminder_days < 1 ) {
            return;
        }

        $table  = $wpdb->prefix . 'themisdb_licenses';
        $today  = gmdate( 'Y-m-d' );
        $cutoff = gmdate( 'Y-m-d', strtotime( "+{$reminder_days} days" ) );

        // Find active licenses expiring within the reminder window
        $licenses = $wpdb->get_results(
            $wpdb->prepare(
                "SELECT l.*, o.customer_name, o.customer_email, o.customer_company
                 FROM {$table} l
                 LEFT JOIN {$wpdb->prefix}themisdb_orders o ON o.id = l.order_id
                 WHERE l.license_status = 'active'
                   AND l.expiry_date IS NOT NULL
                   AND l.expiry_date != '9999-12-31'
                   AND l.expiry_date > %s
                   AND l.expiry_date <= %s",
                $today,
                $cutoff
            ),
            ARRAY_A
        );

        foreach ( $licenses as $license ) {
            self::maybe_send_reminder( $license, $today );
        }
    }

    /**
     * Send a reminder for a single license if one has not already been sent today.
     *
     * @param array  $license  License row with joined customer columns.
     * @param string $today    Today's date (Y-m-d).
     */
    private static function maybe_send_reminder( array $license, string $today ) {
        global $wpdb;

        // Decode usage_data to check last reminder date
        $usage_data = ! empty( $license['usage_data'] )
            ? json_decode( $license['usage_data'], true )
            : array();

        if ( ! is_array( $usage_data ) ) {
            $usage_data = array();
        }

        $last_sent = $usage_data['renewal_reminder_sent_date'] ?? '';
        if ( $last_sent === $today ) {
            return; // Already sent today
        }

        $sent = self::send_reminder_email( $license );

        if ( $sent ) {
            // Record the date so we don't send again today
            $usage_data['renewal_reminder_sent_date'] = $today;
            $wpdb->update(
                $wpdb->prefix . 'themisdb_licenses',
                array( 'usage_data' => wp_json_encode( $usage_data ) ),
                array( 'id' => (int) $license['id'] ),
                array( '%s' ),
                array( '%d' )
            );
        }
    }

    /**
     * Send the renewal reminder e-mail to the customer.
     *
     * @param array $license  License row with joined customer columns.
     * @return bool           True if wp_mail succeeded.
     */
    private static function send_reminder_email( array $license ) {
        $to = $license['customer_email'] ?? '';
        if ( empty( $to ) || ! is_email( $to ) ) {
            return false;
        }

        $expiry_date  = $license['expiry_date'];
        $days_left    = (int) floor( ( strtotime( $expiry_date . ' 00:00:00' ) - strtotime( gmdate( 'Y-m-d' ) . ' 00:00:00' ) ) / DAY_IN_SECONDS );
        $edition      = strtoupper( $license['product_edition'] ?? 'COMMUNITY' );
        $license_key  = $license['license_key'] ?? '';
        $customer     = $license['customer_name'] ?? $to;
        $company      = $license['customer_company'] ?? '';

        $subject = sprintf(
            /* translators: %1$s = edition, %2$d = days left */
            __( '[ThemisDB] Your %1$s license expires in %2$d day(s)', 'themisdb-order-request' ),
            $edition,
            $days_left
        );

        $renewal_url = apply_filters(
            'themisdb_renewal_url',
            site_url( '/contact/' ),
            $license
        );

        $message = self::get_reminder_template( array(
            'customer'    => $customer,
            'company'     => $company,
            'edition'     => $edition,
            'license_key' => $license_key,
            'expiry_date' => $expiry_date,
            'days_left'   => $days_left,
            'renewal_url' => $renewal_url,
        ) );

        $from_email = get_option( 'themisdb_order_email_from', get_option( 'admin_email' ) );
        $from_name  = get_option( 'themisdb_order_email_from_name', get_option( 'blogname' ) );

        $headers = array(
            'Content-Type: text/html; charset=UTF-8',
            sprintf( 'From: %s <%s>', $from_name, $from_email ),
        );

        return wp_mail( $to, $subject, $message, $headers );
    }

    /**
     * Build the HTML e-mail body for a renewal reminder.
     *
     * @param array $data  Template variables.
     * @return string      HTML e-mail body.
     */
    private static function get_reminder_template( array $data ) {
        $site_name = get_option( 'blogname' );
        $site_url  = home_url();

        ob_start();
        ?>
<!DOCTYPE html>
<html>
<head><meta charset="UTF-8"></head>
<body style="font-family:Arial,sans-serif;max-width:600px;margin:0 auto;padding:20px;">
    <h2 style="color:#333;"><?php esc_html_e( 'License Renewal Reminder', 'themisdb-order-request' ); ?></h2>

    <p><?php printf(
        /* translators: customer name */
        esc_html__( 'Dear %s,', 'themisdb-order-request' ),
        esc_html( $data['customer'] )
    ); ?></p>

    <p><?php printf(
        /* translators: %1$s = edition, %2$d = days, %3$s = date */
        esc_html__( 'Your ThemisDB %1$s license will expire in <strong>%2$d day(s)</strong> on %3$s.', 'themisdb-order-request' ),
        '<strong>' . esc_html( $data['edition'] ) . '</strong>',
        (int) $data['days_left'],
        '<strong>' . esc_html( $data['expiry_date'] ) . '</strong>'
    ); ?></p>

    <table style="border-collapse:collapse;width:100%;margin:20px 0;">
        <tr style="background:#f5f5f5;">
            <th style="text-align:left;padding:8px;border:1px solid #ddd;"><?php esc_html_e( 'License Key', 'themisdb-order-request' ); ?></th>
            <td style="padding:8px;border:1px solid #ddd;"><code><?php echo esc_html( $data['license_key'] ); ?></code></td>
        </tr>
        <tr>
            <th style="text-align:left;padding:8px;border:1px solid #ddd;"><?php esc_html_e( 'Edition', 'themisdb-order-request' ); ?></th>
            <td style="padding:8px;border:1px solid #ddd;"><?php echo esc_html( $data['edition'] ); ?></td>
        </tr>
        <tr style="background:#f5f5f5;">
            <th style="text-align:left;padding:8px;border:1px solid #ddd;"><?php esc_html_e( 'Expiry Date', 'themisdb-order-request' ); ?></th>
            <td style="padding:8px;border:1px solid #ddd;"><?php echo esc_html( $data['expiry_date'] ); ?></td>
        </tr>
        <?php if ( ! empty( $data['company'] ) ) : ?>
        <tr>
            <th style="text-align:left;padding:8px;border:1px solid #ddd;"><?php esc_html_e( 'Organization', 'themisdb-order-request' ); ?></th>
            <td style="padding:8px;border:1px solid #ddd;"><?php echo esc_html( $data['company'] ); ?></td>
        </tr>
        <?php endif; ?>
    </table>

    <p style="margin:24px 0;">
        <a href="<?php echo esc_url( $data['renewal_url'] ); ?>"
           style="background:#0073aa;color:#fff;padding:12px 24px;text-decoration:none;border-radius:4px;display:inline-block;">
            <?php esc_html_e( 'Renew License Now', 'themisdb-order-request' ); ?>
        </a>
    </p>

    <p style="color:#777;font-size:12px;">
        <?php printf(
            /* translators: %s = site name */
            esc_html__( 'This message was sent automatically by %s.', 'themisdb-order-request' ),
            esc_html( $site_name )
        ); ?>
        <br>
        <a href="<?php echo esc_url( $site_url ); ?>"><?php echo esc_url( $site_url ); ?></a>
    </p>
</body>
</html>
        <?php
        return ob_get_clean();
    }
}
