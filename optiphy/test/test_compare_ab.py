import numpy as np
import pytest

from optiphy.ana.compare_ab import (
    chi2_1d,
    compare_hits,
    hit_parser,
    two_proportion_z_score,
)


def test_chi2_1d_ignores_overall_normalization():
    """Proportional histograms should match despite different sample totals."""
    a = np.array([0.1, 0.2, 1.1, 1.2, 1.3, 1.4])
    b = np.array([0.1, 0.2, 0.3, 1.1, 1.2, 1.3, 1.4, 1.5, 1.6])

    chi2, ndf = chi2_1d(a, b, bins=[0.0, 1.0, 2.0])

    assert chi2 == pytest.approx(0.0)
    assert ndf == 1


def test_chi2_1d_detects_shape_difference():
    """Different bin proportions should produce a nonzero shape statistic."""
    a = np.array([0.1, 0.2, 0.3, 0.4, 1.1, 1.2])
    b = np.array([0.1, 0.2, 1.1, 1.2, 1.3, 1.4])

    chi2, ndf = chi2_1d(a, b, bins=[0.0, 1.0, 2.0])

    assert chi2 == pytest.approx(4.0 / 3.0)
    assert ndf == 1


def test_two_proportion_z_score_accepts_equal_rates():
    """Equal hit fractions should have zero statistical separation."""
    assert two_proportion_z_score(250, 1000, 2500, 10000) == pytest.approx(0.0)


def test_two_proportion_z_score_detects_rate_difference():
    """The count test should measure differing efficiencies, not raw totals."""
    assert two_proportion_z_score(226, 1000, 296, 1000) == pytest.approx(3.56402, rel=1e-5)


@pytest.mark.parametrize(
    ("successes", "trials"),
    [(-1, 100), (101, 100), (0, 0)],
)
def test_two_proportion_z_score_rejects_invalid_counts(successes, trials):
    """Invalid hit and launched-photon counts should be rejected explicitly."""
    with pytest.raises(ValueError):
        two_proportion_z_score(successes, trials, 50, 100)


@pytest.mark.parametrize(
    ("extra_args", "expected_status"),
    [([], 1), (["--report-only"], 0)],
)
def test_report_only_preserves_default_enforcement(tmp_path, extra_args, expected_status):
    """Statistical mismatches should remain fatal unless explicitly diagnostic."""
    g4_path = tmp_path / "g_hits.npy"
    gpu_path = tmp_path / "s_hits.npy"
    np.save(g4_path, np.zeros((1, 4, 4)))
    np.save(gpu_path, np.zeros((9, 4, 4)))
    args = hit_parser().parse_args(
        [
            str(g4_path),
            str(gpu_path),
            "--count-trials",
            "10",
            "--count-nsigma",
            "3",
            *extra_args,
        ]
    )

    assert compare_hits(args) == expected_status


@pytest.mark.parametrize(
    ("extra_args", "expected_status"),
    [([], 1), (["--report-only"], 0)],
)
def test_report_only_applies_to_shape_mismatches(tmp_path, extra_args, expected_status):
    """Diagnostic mode should report differing shapes without weakening its default."""
    g4_path = tmp_path / "g_hits.npy"
    gpu_path = tmp_path / "s_hits.npy"
    g4_hits = np.zeros((10, 4, 4))
    gpu_hits = np.zeros((10, 4, 4))
    gpu_hits[:, 0, 0] = 1.0
    np.save(g4_path, g4_hits)
    np.save(gpu_path, gpu_hits)
    args = hit_parser().parse_args(
        [
            str(g4_path),
            str(gpu_path),
            "--count-trials",
            "10",
            "--count-nsigma",
            "3",
            "--chi2-ndf-tolerance",
            "5",
            *extra_args,
        ]
    )

    assert compare_hits(args) == expected_status


def test_report_only_does_not_suppress_required_hit_failure(tmp_path):
    """Diagnostic statistics should still fail when a required hit input is empty."""
    g4_path = tmp_path / "g_hits.npy"
    gpu_path = tmp_path / "s_hits.npy"
    np.save(g4_path, np.zeros((0, 4, 4)))
    np.save(gpu_path, np.zeros((1, 4, 4)))
    args = hit_parser().parse_args(
        [
            str(g4_path),
            str(gpu_path),
            "--count-trials",
            "10",
            "--count-nsigma",
            "3",
            "--report-only",
            "--require-hits",
        ]
    )

    assert compare_hits(args) == 1
