"""Focused synchronous vector-search contract regressions."""

import json

import httpx
import pytest

from themis import ThemisClient


PRIVATE_ENDPOINT = str(httpx.URL("http://a").copy_with(
    username="sdk-user", path="/private", **{"password": "example"},
    query=b"token=test-token", fragment="test-fragment",
))


def search(payloads, *, top_k=10, **kwargs):
    def handler(request):
        return httpx.Response(200, json=payloads[request.url.host])

    with ThemisClient(
        [f"http://{host}" for host in payloads],
        transport=httpx.MockTransport(handler),
    ) as client:
        return client.vector_search([0.1, 0.2], top_k=top_k, **kwargs)


def test_distance_only_shards_rank_ascending_and_stably():
    payloads = {
        "a": {"results": [{"pk": "far", "distance": 9}, {"pk": "a", "distance": 1}]},
        "b": {"items": [{"pk": "b", "distance": 1}, {"pk": "near", "distance": 0}]},
    }
    result = search(payloads, top_k=3)
    assert [hit["pk"] for hit in result["results"]] == ["near", "a", "b"]
    assert result["partials"] == list(payloads.values())
    assert "partial" not in result
    assert "errors" not in result


def test_zero_score_does_not_fall_back_to_distracting_distance():
    result = search({
        "a": {"results": [{"pk": "zero", "score": 0, "distance": 100}]},
        "b": {"results": [
            {"pk": "positive", "score": 1},
            {"pk": "negative", "score": -1},
            {"pk": "tied", "score": 0},
        ]},
    })
    assert [hit["pk"] for hit in result["results"]] == [
        "positive", "zero", "tied", "negative",
    ]


def test_common_scores_preferred_to_conflicting_distances():
    result = search({
        "a": {"results": [{"pk": "far", "score": 100, "distance": 5}]},
        "b": {"results": [{"pk": "near", "score": 0, "distance": 1}]},
    })
    assert [hit["pk"] for hit in result["results"]] == ["far", "near"]


def test_null_score_falls_back_to_common_distances():
    result = search({
        "a": {"results": [{"pk": "far", "score": 100, "distance": 5}]},
        "b": {"results": [{"pk": "near", "score": None, "distance": 1}]},
    })
    assert [hit["pk"] for hit in result["results"]] == ["near", "far"]


@pytest.mark.parametrize("second", [
    {"score": 1}, {}, {"distance": None},
])
def test_noncomparable_hits_rejected(second):
    def handler(request):
        hit = {"distance": 1} if request.url.host == "a" else second
        return httpx.Response(200, json={"results": [hit]})

    with ThemisClient(["http://a", "http://b"], transport=httpx.MockTransport(handler)) as client:
        with pytest.raises(ValueError, match="common non-null score or distance"):
            client.vector_search([0.1])


@pytest.mark.parametrize("status", [400, 404, 503])
@pytest.mark.parametrize("hits_key", ["results", "items"])
def test_partial_http_failure_caps_single_success_and_forwards_filter(status, hits_key):
    requests = []
    payload = {
        hits_key: [{"pk": "far", "distance": 9}, {"pk": "near", "distance": 1}],
        "has_more": True,
        "next_cursor": "next",
        "batch_size": 2,
    }

    def handler(request):
        requests.append((request.url.host, json.loads(request.content)))
        if request.url.host == "a":
            return httpx.Response(status, json={"error": "unsupported filter; body-token"})
        return httpx.Response(200, json=payload)

    with ThemisClient(
        [PRIVATE_ENDPOINT, "http://b"],
        max_retries=1,
        transport=httpx.MockTransport(handler),
    ) as client:
        result = client.vector_search(
            [0.1], top_k=1, metadata_filter={"tenant": "blue"},
            use_cursor=True, cursor="previous",
        )
    assert requests == [(host, {
        "vector": [0.1], "k": 1, "filter": {"tenant": "blue"},
        "use_cursor": True, "cursor": "previous",
    }) for host in ("a", "b")]
    assert result[hits_key] == [{"pk": "near", "distance": 1}]
    assert result["has_more"] is True
    assert result["next_cursor"] == "next"
    assert result["batch_size"] == 1
    assert result["partial"] is True
    assert result["errors"] == [{
        "endpoint": "http://a", "status": status, "message": f"HTTP {status}",
    }]
    assert "body-token" not in json.dumps(result)
    assert "partials" not in result


def test_all_http_failures_raise_with_sanitized_shard_statuses():
    def handler(request):
        return httpx.Response(
            400 if request.url.host == "a" else 503,
            json={"error": "unsupported filter; body-token"},
        )

    with ThemisClient(
        [PRIVATE_ENDPOINT, "http://b"],
        max_retries=1, transport=httpx.MockTransport(handler),
    ) as client:
        with pytest.raises(httpx.HTTPError, match="failed on all shards") as caught:
            client.vector_search([0.1])
    message = str(caught.value)
    assert caught.value.errors == [
        {"endpoint": "http://a", "status": 400, "message": "HTTP 400"},
        {"endpoint": "http://b", "status": 503, "message": "HTTP 503"},
    ]
    assert "400" in message and "503" in message
    assert "http://a" in message and "http://b" in message
    for secret in ("sdk-user", "example", "private", "test-token", "test-fragment", "body-token"):
        assert secret not in message


def test_multiple_successes_and_failure_keep_partials_and_global_top_k():
    payloads = {
        "a": {"results": [{"distance": 5}, {"distance": 1}]},
        "b": {"items": [{"distance": 0}, {"distance": 3}]},
    }

    def handler(request):
        assert request.method == "POST"
        assert request.url.path == "/vector/search"
        assert json.loads(request.content) == {"vector": [0.1], "k": 2, "filter": {}}
        if request.url.host == "c":
            return httpx.Response(400)
        return httpx.Response(200, json=payloads[request.url.host])

    with ThemisClient(
        ["http://a", "http://b", "http://c"], transport=httpx.MockTransport(handler),
    ) as client:
        result = client.vector_search([0.1], top_k=2, metadata_filter={})
    assert result == {
        "results": [{"distance": 0}, {"distance": 1}],
        "partials": list(payloads.values()),
        "partial": True,
        "errors": [{"endpoint": "http://c", "status": 400, "message": "HTTP 400"}],
    }


def test_exhausted_server_retries_retain_status_and_continue_to_next_shard():
    calls = []

    def handler(request):
        calls.append(request.url.host)
        if request.url.host == "a":
            return httpx.Response(503, json={"error": "unavailable"})
        return httpx.Response(200, json={
            "results": [{"score": 1}, {"score": 2}], "count": 2,
        })

    with ThemisClient(
        ["http://a", "http://b"], max_retries=3,
        transport=httpx.MockTransport(handler),
    ) as client:
        result = client.vector_search([0.1], top_k=1)
    assert calls == ["a", "a", "a", "b"]
    assert result == {
        "results": [{"score": 2}], "count": 1, "partial": True,
        "errors": [{"endpoint": "http://a", "status": 503, "message": "HTTP 503"}],
    }


def test_nontransport_request_failure_collected():
    def handler(request):
        if request.url.host == "a":
            raise httpx.DecodingError("private request details", request=request)
        return httpx.Response(200, json={"results": []})

    with ThemisClient(
        ["http://a", "http://b"], max_retries=1,
        transport=httpx.MockTransport(handler),
    ) as client:
        result = client.vector_search([0.1])
    assert result == {
        "results": [], "partial": True,
        "errors": [{
            "endpoint": "http://a", "status": None,
            "message": "Request failure (DecodingError)",
        }],
    }


@pytest.mark.parametrize("all_failed", [False, True])
def test_transport_failures_collected_without_leaking_exception_urls(all_failed):
    def handler(request):
        if request.url.host == "a" or all_failed:
            raise httpx.ConnectError("secret token and private URL", request=request)
        return httpx.Response(200, json={"results": [{"distance": 0}]})

    with ThemisClient(
        [PRIVATE_ENDPOINT, "http://b"],
        max_retries=1, transport=httpx.MockTransport(handler),
    ) as client:
        if all_failed:
            with pytest.raises(httpx.HTTPError) as caught:
                client.vector_search([0.1])
            assert "ConnectError" in str(caught.value)
            assert "secret" not in str(caught.value)
            assert caught.value.errors == [
                {"endpoint": f"http://{host}", "status": None,
                 "message": "Request failure (ConnectError)"}
                for host in ("a", "b")
            ]
        else:
            result = client.vector_search([0.1])
            assert result["partial"] is True
            assert result["errors"] == [{
                "endpoint": "http://a", "status": None,
                "message": "Request failure (ConnectError)",
            }]


@pytest.mark.parametrize("failure", [RuntimeError("bug"), ValueError("invalid payload")])
def test_unrelated_exceptions_propagate(failure):
    def handler(request):
        if isinstance(failure, ValueError):
            return httpx.Response(200, content="not JSON")
        raise failure

    with ThemisClient(["http://a"], transport=httpx.MockTransport(handler)) as client:
        with pytest.raises(type(failure)):
            client.vector_search([0.1])


@pytest.mark.parametrize("metadata_filter", [None, {}])
def test_single_endpoint_preserves_payload_and_empty_filter(metadata_filter):
    payload = {"results": [{"distance": 0}], "count": 1, "k": 10}

    def handler(request):
        body = json.loads(request.content)
        assert body == {"vector": [0.1], "k": 10, **(
            {"filter": metadata_filter} if metadata_filter is not None else {}
        )}
        return httpx.Response(200, json=payload)

    with ThemisClient(["http://a"], transport=httpx.MockTransport(handler)) as client:
        assert client.vector_search([0.1], metadata_filter=metadata_filter) == payload


@pytest.mark.parametrize("hits_key", ["results", "items"])
def test_single_endpoint_top_k_and_count(hits_key):
    result = search({"a": {
        hits_key: [{"score": 0}, {"score": 2}], "count": 2, "batch_size": 2,
        "k": 1, "has_more": True, "next_cursor": "next",
    }}, top_k=1)
    assert result == {
        hits_key: [{"score": 2}], "count": 1, "batch_size": 1,
        "k": 1, "has_more": True, "next_cursor": "next",
    }


def test_empty_success_is_not_a_failure():
    assert search({"a": {"results": []}, "b": {"items": []}}) == {
        "results": [], "partials": [{"results": []}, {"items": []}],
    }
