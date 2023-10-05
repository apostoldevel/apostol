--------------------------------------------------------------------------------
-- public.currency -------------------------------------------------------------
--------------------------------------------------------------------------------

CREATE TABLE public.currency (
    id          uuid PRIMARY KEY,
    code        text NOT NULL,
    name        text NOT NULL
);

COMMENT ON TABLE public.currency IS 'Валюта.';

COMMENT ON COLUMN public.currency.id IS 'Идентификатор.';
COMMENT ON COLUMN public.currency.code IS 'Код.';
COMMENT ON COLUMN public.currency.name IS 'Наименование.';

CREATE UNIQUE INDEX ON public.currency (code);
