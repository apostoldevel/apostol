--------------------------------------------------------------------------------
-- FUNCTION public.min_date ----------------------------------------------------
--------------------------------------------------------------------------------

CREATE OR REPLACE FUNCTION public.min_date() RETURNS DATE
AS $$
BEGIN
  RETURN TO_DATE('1970-01-01', 'YYYY-MM-DD');
END;
$$ LANGUAGE plpgsql IMMUTABLE;

--------------------------------------------------------------------------------
-- FUNCTION public.max_date ----------------------------------------------------
--------------------------------------------------------------------------------

CREATE OR REPLACE FUNCTION public.max_date() RETURNS DATE
AS $$
BEGIN
  RETURN TO_DATE('4433-12-31', 'YYYY-MM-DD');
END;
$$ LANGUAGE plpgsql IMMUTABLE;
